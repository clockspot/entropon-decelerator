// ==========================================
// DisplayManager.h - Shared display code
// ==========================================

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#ifdef EXPANDER_ADDRESS //only control unit has this
  #include <Wire.h>
  #include <PCF8574.h> // Pin expander for analog clocks
#endif
#ifdef PIN_DIGITAL_NOR_CLK
    #include <TM1637TinyDisplay6.h>  // For digital clocks
#endif
#include "TimeTypes.h"
#include "config.h"

class DisplayManager {
private:
    //See updateDigitalClock()
    uint32_t digitalClockCurrent[4]; //We will only update when time/blink changes
    bool digitalClockCurrentBlink[2]; //only the first two clocks blink
    #ifdef PIN_DIGITAL_NOR_CLK
        TM1637TinyDisplay6* digitalClocks[4];
    #endif

    #ifdef EXPANDER_ADDRESS
        PCF8574* expander;
        //See cycleAnalogClocks()
        uint8_t analogClockCurrent[3];
        uint8_t analogClockTarget[3];
        unsigned long analogClockLastTick[3];
    #endif

public:
    DisplayManager() {
        #ifdef PIN_DIGITAL_NOR_CLK
            digitalClocks[0] = new TM1637TinyDisplay6(PIN_DIGITAL_NOR_CLK, PIN_DIGITAL_NOR_DIO);
            digitalClocks[1] = new TM1637TinyDisplay6(PIN_DIGITAL_CHM_CLK, PIN_DIGITAL_CHM_DIO);
            digitalClocks[2] = new TM1637TinyDisplay6(PIN_DIGITAL_SAV_CLK, PIN_DIGITAL_SAV_DIO);
            digitalClocks[3] = new TM1637TinyDisplay6(PIN_DIGITAL_ELP_CLK, PIN_DIGITAL_ELP_DIO);
        #endif
        #ifdef EXPANDER_ADDRESS
            expander = new PCF8574(EXPANDER_ADDRESS);        
        #endif
    }
    
    void begin() {
        //Force digital clocks to change
        for(int i=0; i<4; i++) {
            digitalClockCurrent[i] = 60; //not a real time! you can't have 60 secs!
        }
        #ifdef PIN_DIGITAL_NOR_CLK
            //Initialize digital clocks
            for(int i=0; i<4; i++) {
                digitalClocks[i]->begin();
                switch(i) {
                    case 0: digitalClocks[i]->setBrightness(DIGITAL_NOR_BRIGHTNESS); break;
                    case 1: digitalClocks[i]->setBrightness(DIGITAL_CHM_BRIGHTNESS); break;
                    case 2: digitalClocks[i]->setBrightness(DIGITAL_SAV_BRIGHTNESS); break;
                    case 3: digitalClocks[i]->setBrightness(DIGITAL_ELP_BRIGHTNESS); break;
                }
                digitalClocks[i]->clear();
            }
        #endif

        #ifdef EXPANDER_ADDRESS
            //Initialize expander for analog clocks
            Wire.begin();
            expander->begin();
            expander->selectNone(); //set all pins low
        #endif

        #ifdef PIN_RELAY_DECEL
            pinMode(PIN_RELAY_DECEL,    OUTPUT); digitalWrite(PIN_RELAY_DECEL,    LOW);
        #endif
        
        // Set up LED pins
        pinMode(PIN_LED_DECEL,    OUTPUT); digitalWrite(PIN_LED_DECEL,    LOW);
        pinMode(PIN_LED_RECOVERY, OUTPUT); digitalWrite(PIN_LED_RECOVERY, LOW);
        pinMode(PIN_LED_STABLE,   OUTPUT); digitalWrite(PIN_LED_STABLE,   LOW);
        
        // Set up meter pin
        pinMode(PIN_METER_PWM, OUTPUT);
        analogWrite(PIN_METER_PWM, 0);
    }

    void updateNormalTime(const TimeValue& time) {
        updateDigitalClock(0, time.getHHMMSS(), time.getBlink());
        #ifdef EXPANDER_ADDRESS
            analogClockTarget[0] = time.getTotalSeconds()%60;
        #endif
    }

    void updateChamberTime(const TimeValue& time, ExhibitState::State state) {
        updateDigitalClock(1, time.getHHMMSS(), state!=ExhibitState::RECOVERY && time.getBlink()); //do not blink during recovery
        #ifdef EXPANDER_ADDRESS
            analogClockTarget[1] = time.getTotalSeconds()%60;
        #endif
    }
    
    void updateSavedTime(const TimeValue& normal, const TimeValue& chamber) {
        int32_t diffMillis = normal.getDifferenceMillis(chamber);
        int curDifSec = digitalClockCurrent[2]%10000/100;
        // updateDigitalClock(2, (diffMillis/1000/60) * 10000L + (diffMillis/1000%60) * 100L + (diffMillis%1000/10), true); //min.sec.hunds
        updateDigitalClock(2, (diffMillis/1000) * 100L + (diffMillis%1000/10), true); //sec.hunds
        #ifdef EXPANDER_ADDRESS
            //Each time the saved time second increases (including rollover), impulse analog clock for accumulated time saved
            if(digitalClockCurrent[2]%10000/100 > curDifSec || curDifSec >= digitalClockCurrent[2]%10000/100 + 59) {
                // Serial.println("DIF PULSE");
                analogClockTarget[2]++; if(analogClockTarget[2]>=60) analogClockTarget[2]=0;
            }
        #endif
    }
    
    void updateElapsedTime(uint32_t elapsedMillis) {
        // updateDigitalClock(3, (elapsedMillis/1000/3600%100) * 10000L + (elapsedMillis/1000/60%60) * 100L + (elapsedMillis/1000%60), true); //hr.min.sec
        updateDigitalClock(3, (elapsedMillis/1000) * 100L + (elapsedMillis%1000/10), true); //sec.hunds
    }

    void displayDiagnostics(uint32_t code) {
        updateDigitalClock(3, code, false);
    }
    
    // Update analog meter with PWM
    void updateMeter(uint16_t rateMs) {
        // Convert milliseconds per second to seconds lost per second
        // 1000 ms/s = 0 seconds lost
        // 500 ms/s = 0.5 seconds lost  
        // 0 ms/s = 1 second lost
        
        uint8_t pwmValue;
        if (rateMs >= 1000) {
            pwmValue = 0;
        } else {
            pwmValue = (METER_MAX * (1000 - rateMs)) / 1000;
        }
        
        analogWrite(PIN_METER_PWM, pwmValue);
    }
    
    void updateVibes(uint16_t rateMs) {
    #ifdef PIN_VIBES_PWM
        // Convert milliseconds per second to seconds lost per second
        // 1000 ms/s = 0 seconds lost
        // 500 ms/s = 0.5 seconds lost  
        // 0 ms/s = 1 second lost
        
        uint8_t pwmValue;
        if (rateMs >= 1000) {
            pwmValue = 0;
        } else {
            pwmValue = (METER_MAX * (1000 - rateMs)) / 1000;
        }
        
        analogWrite(PIN_VIBES_PWM, pwmValue);
    #endif
    }
    
    void updateLEDs(ExhibitState::State state, const TimeValue& time) {
        //Formerly used time.getBlink() or time.getLongBlink() for some of these, but stopped bc 1) it doesn't look great with our dicey timing and 2) if steady, can do double duty as relay switch
        switch (state) {
            case ExhibitState::NORMAL: default:
                digitalWrite(PIN_LED_DECEL,    LOW);
                digitalWrite(PIN_LED_RECOVERY, LOW);
                digitalWrite(PIN_LED_STABLE,   HIGH);
                break;
            case ExhibitState::DECELERATION:
                digitalWrite(PIN_LED_DECEL,    HIGH);
                digitalWrite(PIN_LED_RECOVERY, LOW);
                digitalWrite(PIN_LED_STABLE,   LOW);
                break;
            case ExhibitState::RECOVERY:
                digitalWrite(PIN_LED_DECEL,    LOW);
                digitalWrite(PIN_LED_RECOVERY, HIGH);
                digitalWrite(PIN_LED_STABLE,   LOW);
                break;
        }
    }

    void updateRelays(ExhibitState::State state) {
        switch (state) {
            case ExhibitState::DECELERATION:
                #ifdef PIN_RELAY_DECEL
                    digitalWrite(PIN_RELAY_DECEL, HIGH);
                #endif
                break;
            case ExhibitState::NORMAL:
            case ExhibitState::RECOVERY:
            default:
                #ifdef PIN_RELAY_DECEL
                    digitalWrite(PIN_RELAY_DECEL, LOW);
                #endif
                break;
        }
    }

    void cycleAnalogClocks() {
    #ifdef EXPANDER_ADDRESS
        //Run on every loop. Controls advance of clocks to targets by switching pins per pulse width and max tick rate.
        //TODO if delays of over 60sec occur, it will sync up, but a minute behind. Solution: track more than secs?
        //clock 0 is pins 0 (even) and 1 (odd); 1 is 2 and 3; 2 is 4 and 5. Thus pin is i*2+(sec%2)
        unsigned long startMils = 0;
        unsigned long diffMils = 0;
        bool analogClockPinState[6];
        for(int i=0; i<6; i++) analogClockPinState[i]=0;
        //Check if a tick is needed
        for(int i=0; i<3; i++) {
            if(analogClockCurrent[i] != analogClockTarget[i]) {
                if(millis()-analogClockLastTick[i] > ANALOG_MAX_TICK_RATE) {
                    analogClockCurrent[i]++; if(analogClockCurrent[i]>=60) analogClockCurrent[i]=0;
                    analogClockLastTick[i] = millis();
                    analogClockPinState[i*2+(analogClockCurrent[i]%2)] = true;
                }
            }
        }
        //Switch pins on
        for(int i=0; i<6; i++) {
            if(analogClockPinState[i]) {
                if(startMils==0) startMils = millis();
                expander->write(i, HIGH);
            }
        }
        delay(ANALOG_PULSE_WIDTH); //The only place this is acceptable since the timing of the pulse is so crucial, it can't be left to loop polling - it's preferable for the other code to hang, with implications for button polling (TODO need to implement as interrupts) and display refresh rate (which for the TM1637s is slow anyway)
        //Switch pins off
        for(int i=0; i<6; i++) {
            if(analogClockPinState[i]) {
                if(startMils!=0) diffMils = millis() - startMils;
                expander->write(i, LOW);
            }
        }
        //The digital clocks do not need a similar function, as the TM1637 handles this.
    #endif
    }
    
    void testPattern(int networkState) {
        // All on, all eights, needle rise
        // All LEDs on [wip]
        // for (int i = 0; i < 3; i++) {
        //     digitalWrite(ledPins[i], HIGH);
        // }
        digitalWrite(PIN_LED_DECEL,    HIGH);
        digitalWrite(PIN_LED_RECOVERY, HIGH);
        digitalWrite(PIN_LED_STABLE,   HIGH);
        #ifdef PIN_DIGITAL_NOR_CLK
            for (int i = 0; i < 4; i++) digitalClocks[i]->showString("888888");
        #endif
        //TODO better way to make the needle move more gently
        analogWrite(PIN_METER_PWM, (METER_MAX*1)/5);
        delay(200);
        analogWrite(PIN_METER_PWM, (METER_MAX*2)/5);
        delay(200);
        analogWrite(PIN_METER_PWM, (METER_MAX*3)/5);
        delay(200);
        analogWrite(PIN_METER_PWM, (METER_MAX*4)/5);
        delay(200);
        analogWrite(PIN_METER_PWM, METER_MAX);
        
        #ifdef PIN_DIGITAL_NOR_CLK
            digitalClocks[0]->showString("nor");
            digitalClocks[1]->showString("CHA");
            digitalClocks[2]->showString("SAU");
            digitalClocks[3]->showString("ELP");
        #endif
        delay(5000);
        
        // Clear everything
        //TODO better way to make the needle move more gently
        analogWrite(PIN_METER_PWM, (METER_MAX*4)/5);
        delay(200);
        analogWrite(PIN_METER_PWM, (METER_MAX*3)/5);
        delay(200);
        analogWrite(PIN_METER_PWM, (METER_MAX*2)/5);
        delay(200);
        analogWrite(PIN_METER_PWM, (METER_MAX*1)/5);
        delay(200);
        analogWrite(PIN_METER_PWM, 0);
        digitalWrite(PIN_LED_DECEL,    LOW);
        digitalWrite(PIN_LED_RECOVERY, LOW);
        digitalWrite(PIN_LED_STABLE,   LOW);
        #ifdef PIN_DIGITAL_NOR_CLK
            for (int i = 0; i < 4; i++) digitalClocks[i]->showString("------");
            if(networkState) digitalClocks[3]->showString(networkState>1? "WiFi Y": "WiFi n");
        #endif
    }
    
private:
    void updateDigitalClock(int i, int32_t dec, bool colons) {
        //Sends update to TM1637 when display has changed
        if(digitalClockCurrent[i] != dec || (i<2 && digitalClockCurrentBlink[i] != colons)) {
            // if(i>1) {
                // Serial.print(i,DEC);
                // Serial.print(" ");
                // Serial.print(dec,DEC);
            // }
            digitalClockCurrent[i] = dec;
            // Serial.print(" ");
            // Serial.print(digitalClockCurrent[i],DEC);
            // Serial.println();

            if(i<2) digitalClockCurrentBlink[i] = colons;
            #ifdef PIN_DIGITAL_NOR_CLK
                digitalClocks[i]->showNumberDec(
                    digitalClockCurrent[i],
                    ((i<2? 1-digitalClockCurrentBlink[i] : colons)? (i<2? 0b01010000: 0b00010000) : 0b00000000),
                    true, 6
                );
            #endif
        }
    }

};

#endif // DISPLAY_MANAGER_H