// ControlUnit.ino
#include <Wire.h>
#include <RTClib.h>
// #include <PCF8574.h>
// #include <SoftwareSerial.h>
//https://forum.arduino.cc/t/using-additional-serial-ports/605955
//https://learn.adafruit.com/using-atsamd21-sercom-to-add-more-spi-i2c-serial-ports/overview
#include "config_control.h"
#include "TimeTypes.h"
#include "DisplayManager.h"
#include "SerialProtocol.h"

// Global state
TimeValue outsideTime;
TimeValue chamberTime;
ExhibitState state;
DisplayManager display( //TODO incorporate per config TODO can the clk pin be shared?
  DIGITAL_PCF8574_ADDRESS,
  // PIN_LED_NORMAL, PIN_LED_DECEL, PIN_LED_RECOVERY, //LEDs
  PIN_METER_PWM //, //Meter
  // PIN_ANALOG_OUT_A, PIN_ANALOG_OUT_B, //Analog outside time
  // PIN_ANALOG_CHM_A, PIN_ANALOG_CHM_B, //Analog chamber time
  // PIN_ANALOG_DIF_A, PIN_ANALOG_DIF_B //Analog difference (saved) time
);

#ifdef RTC_ENABLED
  RTC_DS3231 rtc;
#endif

// SerialProtocol protocol;
#ifdef PIN_CHAMBER_TX
  SoftwareSerial chamberSerial(PIN_CHAMBER_RX, PIN_CHAMBER_TX);
  SerialProtocol chamberUnit(&chamberSerial);
#endif
#ifdef PIN_PRINTER_TX
  SoftwareSerial printerSerial(PIN_PRINTER_RX, PIN_PRINTER_TX);
#endif

// Clock pulse tracking
bool outsideClockOdd = false;
bool chamberClockOdd = false;
bool savedClockOdd = false;
uint8_t lastOutsideSecond = 255;
uint8_t lastChamberSecond = 255;
uint32_t lastSavedSeconds = 0;

// Timing
uint32_t lastLoopMillis = 0;

void setup() {
  delay(2000);
    Serial.begin(115200);
    Serial.println("");
    Serial.println("Hello world");

    #ifdef PIN_CHAMBER_TX
      chamberSerial.begin(9600);
    #endif
    #ifdef PIN_PRINTER_TX
      printerSerial.begin(9600);
    #endif
    
    // Initialize inputs
    pinMode(PIN_START_BUTTON,INPUT_PULLUP);
    pinMode(PIN_STOP_BUTTON,INPUT_PULLUP);
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
    outsideTime.addMillis(deltaMillis);
    
    // Calculate chamber time rate based on state
    // updateChamberRate();
    
    // Calculate chamber time delta using the rate
    // chamberDelta = deltaMillis * (chamberRateMs / 1000)
    // Using integer math: chamberDelta = (deltaMillis * chamberRateMs) / 1000
    uint32_t chamberDelta = ((uint32_t)deltaMillis * state.chamberRateMs) / 1000;
    chamberTime.addMillis(chamberDelta);
    
    // State machine
    handleStateTransitions();
    
    // Update displays
    display.updateDigitalClock(0,outsideTime);
    display.updateDigitalClock(1,chamberTime);
    display.updateDigitalClockDifference(outsideTime,chamberTime); //TODO does this deal with rollover?
    if(state.current == ExhibitState::DECELERATION) display.updateDigitalClockElapsed(state.elapsedMillis);
    display.updateMeter(state.chamberRateMs);
    display.updateLEDs(state.current);
    display.updateAnalogClocks();

    // Communicate with chamber unit
    #ifdef PIN_CHAMBER_TX
      chamberUnit.sendStateUpdate(state);
      chamberUnit.sendTimeUpdate(outsideTime, chamberTime); //TODO why not other values?
    
      // Check for stop signal from chamber
      uint8_t msgType, payload[32];
      if (protocol.receiveMessage(&msgType, payload)) {
          if (msgType == SerialProtocol::MSG_STOP) {
              triggerStop();
          }
      }
    #endif
    
    // Re-sync with RTC at midnight
    #ifdef RTC_ENABLED
      if (state.current == ExhibitState::NORMAL && outsideTime.getHHMMSS() < 2) {
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
      uint16_t potMaxNeg = 512;
    // #endif

    // #ifdef PIN_POT_MAX_POS
    //   uint16_t potMaxPos = analogRead(PIN_POT_MAX_POS);
    // #else
      uint16_t potMaxPos = 512;
    // #endif

    // #ifdef PIN_POT_MIN_RATE
    //   uint16_t potMinRate = analogRead(PIN_POT_MIN_RATE);
    // #else
      uint16_t potMinRate = 512;
    // #endif

    // Minimum rate from pot (100-1000 ms per second)
    uint16_t minRate = RATE_MIN; // + ((uint32_t)potMinRate * 1000-RATE_MIN) / 1023;

    //The chamber rate will be somewhere between 1000 and RATE_MIN (likely 100) ms/sec.
    //The change rate will be somewhere between POWER (eg 20) and 1 ms/sec/sec.

    //Linear interpolation
    uint8_t changeRate = 1 + (state.chamberRateMs - minRate) * (20 - 1) / (1000 - minRate);


    
    // Deceleration/recovery duration from pots (2-20 seconds)
    uint32_t decelDuration = 2000 + ((uint32_t)(1023 - potMaxNeg) * 18000) / 1023;
    uint32_t recoveryDuration = 2000 + ((uint32_t)(1023 - potMaxPos) * 18000) / 1023;
    
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
            if (state.elapsedMillis >= recoveryDuration) {
                // Back to normal
                state.chamberRateMs = 1000;
            } else {
                // Linear interpolation from savedMinRate to 1000
                uint32_t progress = (state.elapsedMillis * 1000) / recoveryDuration;
                state.chamberRateMs = state.savedMinRate + 
                    ((1000 - state.savedMinRate) * progress) / 1000;
            }
            break;
        }
    }
}

void handleStateTransitions() {
    static uint32_t decelStartTime = 0;
    static uint32_t recoverStartTime = 0;
    
    switch (state.current) {
        case ExhibitState::NORMAL:
            if (digitalRead(PIN_START_BUTTON) == LOW) {
                state.current = ExhibitState::DECELERATION;
                Serial.println("Decelerating");
                state.elapsedMillis = 0;
                state.chamberRateMs = 800;
                decelStartTime = millis();
            }
            break;
            
        case ExhibitState::DECELERATION:
            state.elapsedMillis = millis() - decelStartTime;
            
            if (digitalRead(PIN_STOP_BUTTON) == LOW || 
              state.chamberRateMs <= 110) {  // Within 10% of minimum
              //TODO that's not what I meant
                state.current = ExhibitState::RECOVERY;
                Serial.println("Recovering");
                state.elapsedMillis = 0;
                state.chamberRateMs = 1700;
                recoverStartTime = millis();
                printCertificate();
            }
            break;
            
        case ExhibitState::RECOVERY:
            state.elapsedMillis = millis() - recoverStartTime;
            
            if (outsideTime.getDifferenceMillis(chamberTime)<0) {
            // if (state.chamberRateMs >= 1000-(1000/100)) { //within 1% of 1000
                state.chamberRateMs = 1000;
                state.current = ExhibitState::NORMAL;
                Serial.println("Normal");
                state.elapsedMillis = 0;
                // Resync times
                chamberTime = outsideTime;
            }
            break;
    }
}

void printCertificate() {
  #ifdef PIN_PRINTER_TX
    uint32_t timeDiff = abs((int32_t)(outsideTime.millisSinceMidnight - 
                            chamberTime.millisSinceMidnight));
    uint32_t secondsSaved = timeDiff / 1000;
    uint32_t secondsSpent = state.elapsedMillis / 1000;
    
    printerSerial.print(F("I spent "));
    printerSerial.print(secondsSpent);
    printerSerial.print(F(" seconds to save "));
    printerSerial.print(secondsSaved);
    printerSerial.println(F(" seconds in the chamber"));
  #endif
}