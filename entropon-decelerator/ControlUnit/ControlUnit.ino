// ControlUnit.ino

#include "config.h"

#include "TimeTypes.h"
#include "DisplayManager.h"

#ifdef RTC_ENABLED
  #include <Wire.h>
  #include <RTClib.h>
#endif

// #include <SoftwareSerial.h>
#include "SerialProtocol.h"


// Global state
TimeValue normalTime;
TimeValue chamberTime;
ExhibitState state;
DisplayManager display;

#ifdef RTC_ENABLED
  RTC_DS3231 rtc;
#endif

SerialProtocol protocol(&Serial1);

// #ifdef PIN_PRINTER_TX
//   SoftwareSerial printerSerial(PIN_PRINTER_RX, PIN_PRINTER_TX);
// #endif

// Clock pulse tracking
bool normalClockOdd = false;
bool chamberClockOdd = false;
bool savedClockOdd = false;
uint8_t lastNormalSecond = 255;
uint8_t lastChamberSecond = 255;
uint32_t lastSavedSeconds = 0;

// Timing
uint32_t lastLoopMillis = 0;

void setup() {
  #ifdef ENABLE_SERIAL_LOGGING
    Serial.begin(115200);
  #endif
  #ifdef ENABLE_SERIAL_TO_CHAMBER_UNIT
    Serial1.begin(9600); //RX/TX serial to control unit
  #endif

  #ifdef PIN_PRINTER_TX
    printerSerial.begin(9600);
  #endif
  
  // Initialize inputs
  #ifdef PIN_START_BUTTON
    pinMode(PIN_START_BUTTON,INPUT_PULLUP);
  #endif
  #ifdef PIN_STOP_BUTTON
    pinMode(PIN_STOP_BUTTON,INPUT_PULLUP);
  #endif

  //The following may not need init
  //PIN_POT_MAX_NEG
  //PIN_POT_MAX_POS
  //PIN_POT_MIN_RATE

  #ifdef RTC_ENABLED
    // Initialize RTC
    if (!rtc.begin()) {
        Serial.println(F("RTC not found!"));
        while (1) delay(10);
    }
    // Sync time from RTC
    syncTimeFromRTC();
  #endif
      
  // Test displays
  display.begin();
  display.testPattern();

  state.reset();
  lastLoopMillis = millis();
}

void loop() {

  uint32_t currentMillis = millis();
  uint32_t deltaMillis = currentMillis - lastLoopMillis;
  
  // Update times
  normalTime.addMillis(deltaMillis);
  
  // Calculate chamber time rate based on state
  updateChamberRate();
  
  // Calculate chamber time delta using the rate
  // This is a little imprecise over time, but so is Entroponics, really
  uint32_t chamberDelta = ((uint32_t)deltaMillis * state.chamberRateMs) / 1000;
  chamberTime.addMillis(chamberDelta);

  // State machine
  char forceState = 0;
  // Look for state signal from chamber unit
  #ifdef ENABLE_SERIAL_TO_CHAMBER_UNIT
    uint8_t msgType, payload[64];
    uint16_t payloadLength;
    if (protocol.receiveMessage(&msgType, payload, &payloadLength)) {
      #ifdef ENABLE_SERIAL_LOGGING
        Serial.print("Received from chamber: ");
        switch(msgType) {
          case SerialProtocol::MSG_STATE: Serial.print("MSG_STATE"); break;
          case SerialProtocol::MSG_TIME:  Serial.print("MSG_TIME"); break;
          case SerialProtocol::MSG_STOP:  Serial.print("MSG_STOP"); break;
          case SerialProtocol::MSG_ACK:   Serial.print("MSG_ACK"); break;
          case SerialProtocol::MSG_ERROR: Serial.print("MSG_ERROR"); break;
          default: Serial.print("unknown"); break;
        }
        Serial.print(" of length ");
        Serial.println(payloadLength);
      #endif
      if (msgType == SerialProtocol::MSG_STOP) {
        forceState = 2;
      }
    }
  #endif
  handleStateTransitions(forceState);
  
  // Send display data to chamber unit, so it can be updating displays at the same time as control unit
  #ifdef ENABLE_SERIAL_TO_CHAMBER_UNIT
    protocol.sendStateUpdate(state);
    protocol.sendTimeUpdate(normalTime, chamberTime);
  #endif

  // Update local displays
  display.updateLEDs(state.current,normalTime);
  display.updateRelays(state.current);
  display.updateMeter(state.chamberRateMs);
  display.updateNormalTime(normalTime);
  display.updateChamberTime(chamberTime);
  display.updateSavedTime(normalTime,chamberTime); //TODO does this deal with rollover?
  if(state.current == ExhibitState::DECELERATION) {
    display.updateElapsedTime(state.elapsedMillis);
  }
  display.cycleAnalogClocks();
  
  // Re-sync with RTC at midnight
  #ifdef RTC_ENABLED
    if (state.current == ExhibitState::NORMAL && normalTime.getHHMMSS() < 2) {
          //TODO is there a more elegant way to catch this transition?
        syncTimeFromRTC();
    }
  #endif
  
  lastLoopMillis = currentMillis;
}

void updateChamberRate() {
    if(state.current==ExhibitState::NORMAL) return;

    // // Read potentiometers
    // #ifdef PIN_POT_MAX_NEG
    //   uint16_t potMaxNeg = analogRead(PIN_POT_MAX_NEG);
    // #else
      uint16_t potMaxNeg = 0; //512;
    // #endif

    // #ifdef PIN_POT_MAX_POS
    //   uint16_t potMaxPos = analogRead(PIN_POT_MAX_POS);
    // #else
      uint16_t potMaxPos = 512;
    // #endif

    // #ifdef PIN_POT_MIN_RATE
    //   uint16_t potMinRate = analogRead(PIN_POT_MIN_RATE);
    // #else
      uint16_t potMinRate = 1023; //512;
    // #endif



    // Minimum rate from pot (100-1000 ms per second)
    uint16_t minRate = RATE_MIN; // + ((uint32_t)potMinRate * 1000-RATE_MIN) / 1023;

    //The chamber rate will be somewhere between 1000 and RATE_MIN (likely 100) ms/sec.
    //The change rate will be somewhere between POWER (eg 20) and 1 ms/sec/sec.

    //Linear interpolation
    uint8_t changeRate = 1 + (state.chamberRateMs - minRate) * (20 - 1) / (1000 - minRate);


    
    // Deceleration/recovery duration from pots (2-20 seconds)
    uint32_t decelDuration = 2000 + ((uint32_t)(1023 - potMaxNeg) * 180000) / 1023;
    uint32_t recoveryDuration = 2000 + ((uint32_t)(1023 - potMaxPos) * 180000) / 1023;
    
    switch (state.current) {
      case ExhibitState::DECELERATION: {
        if (state.elapsedMillis >= decelDuration) {
            // Reached minimum rate
            state.chamberRateMs = minRate;
        } else {
            // Linear interpolation from 1000 to minRate
            uint32_t progress = (state.elapsedMillis * 1000) / decelDuration;
            state.chamberRateMs = 1000 - ((1000 - minRate) * progress) / 1000;
        }
        break;
      }
      
      case ExhibitState::RECOVERY: {
        state.chamberRateMs = 3000;
          // if (state.elapsedMillis >= recoveryDuration) {
          //     // Back to normal
          //     state.chamberRateMs = 1000;
          // } else {
          //     state.chamberRateMs = 1500;
          //     // // Linear interpolation from savedMinRate to 1000
          //     // uint32_t progress = (state.elapsedMillis * 1000) / recoveryDuration;
          //     // state.chamberRateMs = state.savedMinRate + 
          //     //     ((1000 - state.savedMinRate) * progress) / 1000;
          // }
          // break;
      }
    }
}

void handleStateTransitions(char forceState) {
    static uint32_t decelStartTime = 0;
    static uint32_t recoverStartTime = 0;
    
    switch (state.current) {
        case ExhibitState::NORMAL:
            if (digitalRead(PIN_START_BUTTON) == PIN_START_BUTTON_PRESSED
              || forceState==1
            ) {
                state.current = ExhibitState::DECELERATION;
                #ifdef ENABLE_SERIAL_LOGGING
                  Serial.println("Decelerating");
                #endif
                state.elapsedMillis = 0;
                state.chamberRateMs = 500;
                decelStartTime = millis();
            }
            break;
            
        case ExhibitState::DECELERATION:
            state.elapsedMillis = millis() - decelStartTime;
            
            if (digitalRead(PIN_STOP_BUTTON) == PIN_STOP_BUTTON_PRESSED
              || state.chamberRateMs <= 110 || forceState==2
            ) {  // Within 10% of minimum
              //TODO that's not what I meant
                printCertificate(); //needs state.elapsedMillis to not be zeroed yet
                state.current = ExhibitState::RECOVERY;
                #ifdef ENABLE_SERIAL_LOGGING
                  Serial.println("Recovering");
                #endif
                state.elapsedMillis = 0;
                state.chamberRateMs = 2400;
                recoverStartTime = millis();
            }
            break;
            
        case ExhibitState::RECOVERY:
            state.elapsedMillis = millis() - recoverStartTime;
            
            if (normalTime.getDifferenceMillis(chamberTime)<0) {
            // if (state.chamberRateMs >= 1000-(1000/100)) { //within 1% of 1000
                state.chamberRateMs = 1000;
                state.current = ExhibitState::NORMAL;
                #ifdef ENABLE_SERIAL_LOGGING
                  Serial.println("Normal");
                #endif
                state.elapsedMillis = 0;
                // Resync times
                chamberTime = normalTime;
            }
            break;
    }
}

void printCertificate() {
  uint32_t timeDiff = abs((int32_t)(normalTime.millisSinceMidnight - 
                            chamberTime.millisSinceMidnight));
  uint32_t secondsSaved = timeDiff / 1000;
  uint32_t secondsSpent = state.elapsedMillis / 1000;
  #ifdef PIN_PRINTER_TX    
    printerSerial.print(F("I spent "));
    printerSerial.print(secondsSpent);
    printerSerial.print(F(" seconds to save "));
    printerSerial.print(secondsSaved);
    printerSerial.println(F(" seconds in the chamber"));
  #endif
  #ifdef ENABLE_SERIAL_LOGGING
    Serial.print(F("I spent "));
    Serial.print(secondsSpent);
    Serial.print(F(" seconds to save "));
    Serial.print(secondsSaved);
    Serial.println(F(" seconds in the chamber"));
  #endif
}