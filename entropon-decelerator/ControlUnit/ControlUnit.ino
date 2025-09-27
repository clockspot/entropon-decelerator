// ControlUnit.ino
#include <Wire.h>
#include <RTClib.h>
// #include <SoftwareSerial.h>
//https://forum.arduino.cc/t/using-additional-serial-ports/605955
//https://learn.adafruit.com/using-atsamd21-sercom-to-add-more-spi-i2c-serial-ports/overview
#include "configs/config_control.h"
#include "common/TimeTypes.h"
#include "common/DisplayManager.h"
#include "common/SerialProtocol.h"

// Global state
TimeValue outsideTime;
TimeValue chamberTime;
ExhibitState state;
DisplayManager display( //TODO incorporate per config TODO can the clk pin be shared?
    PIN_DISP_OUT_CLK, PIN_DISP_OUT_DIO, //Digital outside time
    PIN_DISP_CHM_CLK, PIN_DISP_CHM_DIO, //Digital chamber time
    PIN_DISP_DIF_CLK, PIN_DISP_DIF_DIO, //Digital difference
    PIN_DISP_ELP_CLK, PIN_DISP_ELP_DIO, //Digital elapsed
    PIN_LED_NORMAL, PIN_LED_DECEL, PIN_LED_RECOVERY, //LEDs
    PIN_METER_PWM, //Meter
    PIN_ANALOG_OUT_A, PIN_ANALOG_OUT_B, //Analog outside time
    PIN_ANALOG_CHM_A, PIN_ANALOG_CHM_B, //Analog chamber time
    PIN_ANALOG_DIF_A, PIN_ANALOG_DIF_B //Analog difference (saved) time
);

#ifdef(RTC_ENABLED)
  RTC_DS3231 rtc;
#endif

// SerialProtocol protocol;
#ifdef(PIN_CHAMBER_TX)
  SoftwareSerial chamberSerial(PIN_CHAMBER_RX, PIN_CHAMBER_TX);
  SerialProtocol chamberUnit(&chamberSerial);
#endif
#ifdef(PIN_PRINTER_TX)
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
    Serial.begin(115200);
    Serial.println("");
    Serial.println("Hello world");

    #ifdef(PIN_CHAMBER_TX)
      chamberSerial.begin(9600);
    #endif
    #ifdef(PIN_PRINTER_TX)
      printerSerial.begin(9600);
    #endif
    
    // Initialize inputs
    pinMode(PIN_START_BUTTON,INPUT_PULLUP);
    pinMode(PIN_STOP_BUTTON,INPUT_PULLUP);
    //PIN_POT_MAX_NEG
    //PIN_POT_MAX_POS
    //PIN_POT_MIN_RATE
    
    #ifdef(RTC_ENABLED)
      // Initialize RTC
      if (!rtc.begin()) {
          Serial.println(F("RTC not found!"));
          while (1) delay(10);
      }
      // Sync time from RTC
      syncTimeFromRTC();
    #endif
        
    // Test displays
    display.testPattern();
    
    state.current = ExhibitState::NORMAL;
    lastLoopMillis = millis();
}

void loop() {
    uint32_t currentMillis = millis();
    uint32_t deltaMillis = currentMillis - lastLoopMillis;
    
    // Update times
    outsideTime.addMillis(deltaMillis);
    
    // Calculate chamber time rate based on state
    updateChamberRate();
    
    // Update chamber time (using fixed-point math)
    int64_t chamberDelta = ((int64_t)deltaMillis * state.chamberRateQ16) >> 16;
    chamberTime.addMillis(chamberDelta);
    
    // State machine
    handleStateTransitions();
    
    // Update displays
    display.updateTimeDisplay(0,outsideTime.getMillis());
    display.updateTimeDisplay(1,chamberTime.getMillis());
    display.updateDifferenceDisplay(outsideTime.getMillis(),chamberTime.getMillis()); //TODO does this deal with rollover?
    if(state.current == ExhibitState::DECELERATION) display.updateElapsedDisplay(state.elapsedMillis());
    display.updateMeter(state.chamberRateQ16);
    display.updateLEDs(state.current);

    // Communicate with chamber unit
    #ifdef(PIN_CHAMBER_TX)
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
    #ifdef(RTC_ENABLED)
      if (state.current == ExhibitState::NORMAL && outsideTime.getHours() == 0 && 
          outsideTime.getMinutes() == 0 && outsideTime.getSeconds() < 2) {
            //TODO is there a more elegant way to catch this transition?
          syncTimeFromRTC();
      }
    #endif
    
    lastLoopMillis = currentMillis;
}

void updateChamberRate() {
    // Read potentiometers
    float maxNegRate = analogRead(PIN_POT_MAX_NEG) / 1023.0;
    float maxPosRate = analogRead(PIN_POT_MAX_POS) / 1023.0;
    float minChamberRate = analogRead(PIN_POT_MIN_RATE) / 1023.0;
    
    switch (state.current) {
        case ExhibitState::NORMAL:
            state.chamberRateQ16 = 65536; // 1.0 in Q16
            break;
            
        case ExhibitState::DECELERATION: {
            // Smooth exponential decay curve
            float t = state.elapsedMillis / 1000.0;
            float rate = 1.0 - (1.0 - minChamberRate) * (1.0 - exp(-t * maxNegRate));
            state.chamberRateQ16 = (int32_t)(rate * 65536);
            break;
        }
        
        case ExhibitState::RECOVERY: {
            // Smooth exponential recovery curve
            float t = state.elapsedMillis / 1000.0;
            float startRate = ((float)state.chamberRateQ16) / 65536.0;
            float rate = startRate + (1.0 - startRate) * (1.0 - exp(-t * maxPosRate));
            state.chamberRateQ16 = (int32_t)(rate * 65536);
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
                decelStartTime = millis();
            }
            break;
            
        case ExhibitState::DECELERATION:
            state.elapsedMillis = millis() - decelStartTime;
            
            if (digitalRead(PIN_STOP_BUTTON) == LOW || 
                state.chamberRateQ16 <= (int32_t)(0.01 * 65536)) {
                state.current = ExhibitState::RECOVERY;
                Serial.println("Recovering");
                state.elapsedMillis = 0;
                recoverStartTime = millis();
                printCertificate();
            }
            break;
            
        case ExhibitState::RECOVERY:
            state.elapsedMillis = millis() - recoverStartTime;
            
            if (abs(state.chamberRateQ16 - 65536) < 655) { // Within 1% of normal
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
  #ifdef(PIN_PRINTER_TX)
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