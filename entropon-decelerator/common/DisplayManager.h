// ==========================================
// DisplayManager.h - Shared display code
// ==========================================

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <PCF8574.h> // For analog clocks
#include <TM1637TinyDisplay6.h>  // For digital clocks
#include "TimeTypes.h"
#include "config_control.h" //Decided to pull these values in directly since they are immutable

class DisplayManager {
private:
    //See updateDigitalClock()
    TM1637TinyDisplay6* digitalClocks[4];
    uint8_t digitalClockCurrent[4]; //We will only update when time/blink changes
    bool digitalClockCurrentBlink[2]; //only the first two clocks blink

    //See cycleAnalogClocks()
    PCF8574* analogClocks;
    uint8_t analogClockCurrent[3];
    uint8_t analogClockTarget[3];
    bool analogClockEnergized[3]; 
    unsigned long analogClockLastTick[3];

public:
    DisplayManager() {
        digitalClocks[0] = new TM1637TinyDisplay6(PIN_DIGITAL_OUT_CLK, PIN_DIGITAL_OUT_DIO);
        digitalClocks[1] = new TM1637TinyDisplay6(PIN_DIGITAL_CHM_CLK, PIN_DIGITAL_CHM_DIO);
        digitalClocks[2] = new TM1637TinyDisplay6(PIN_DIGITAL_DIF_CLK, PIN_DIGITAL_DIF_DIO);
        digitalClocks[3] = new TM1637TinyDisplay6(PIN_DIGITAL_ELP_CLK, PIN_DIGITAL_ELP_DIO);
        analogClocks = new PCF8574(ANALOG_EXPANDER_ADDRESS);        
    }
    
    void begin() {
        //Initialize digital clocks
        for(int i=0; i<4; i++) {
            digitalClocks[i]->begin();
            switch(i) {
                case 0: digitalClocks[i]->setBrightness(DIGITAL_OUT_BRIGHTNESS); break;
                case 1: digitalClocks[i]->setBrightness(DIGITAL_CHM_BRIGHTNESS); break;
                case 2: digitalClocks[i]->setBrightness(DIGITAL_DIF_BRIGHTNESS); break;
                case 3: digitalClocks[i]->setBrightness(DIGITAL_ELP_BRIGHTNESS); break;
            }
            digitalClocks[i]->clear();
        }

        //Initialize analog clock pins via expander
        Wire.begin();
        analogClocks.begin();
        analogClocks.selectNone(); //set all pins low
        
        // Set up LED pins
        pinMode(PIN_LED_DECEL,    OUTPUT); digitalWrite(PIN_LED_DECEL,    LOW);
        pinMode(PIN_LED_RECOVERY, OUTPUT); digitalWrite(PIN_LED_RECOVERY, LOW);
        pinMode(PIN_LED_STABLE,   OUTPUT); digitalWrite(PIN_LED_STABLE,   LOW);
        
        // Set up meter pin
        pinMode(PIN_METER_PWM, OUTPUT);
        analogWrite(PIN_METER_PWM, 0);
    }

    void updateOutsideTime(const TimeValue& time) {
        updateDigitalClock(0, time.getHHMMSS(), time.getBlink());
        analogClockTarget[0] = time.getSeconds();
    }

    void updateChamberTime(const TimeValue& time) {
        updateDigitalClock(0, time.getHHMMSS(), time.getBlink());
        analogClockTarget[1] = time.getSeconds();
    }
    
    void updateDifferenceTime(const TimeValue& outside, const TimeValue& chamber) {
        int32_t diffMillis = chamber.getDifferenceMillis(outside);
        int curDifSec = digitalClockCurrent[2]%10000/100;
        updateDigitalClock(2, (diffMillis/1000/60) * 10000L + (diffMillis/1000%60) * 100L + (diffMillis%1000/10), true);
        //Each time the difference second increases (including rollover), impulse analog clock for accumulated time saved
        if(digitalClockCurrent[2]%10000/100 > curDifSec || curDifSec >= digitalClockCurrent[2]%10000/100 + 59) {
            analogClockTarget[2]++; if(analogClockTarget[2]>=60) analogClockTarget[2]=0;
        }
    }
    
    void updateElapsedTime(uint32_t elapsedMillis) {
        updateDigitalClock(3, (elapsedMillis/1000/3600%100) * 10000L + (elapsedMillis/1000/60%60) * 100L + (elapsedMillis/1000%60)), true);
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
            // pwmValue = 255 * (1000 - rateMs) / 1000
            pwmValue = (255 * (1000 - rateMs)) / 1000;
        }
        
        analogWrite(meterPin, pwmValue);
    }
    
    void updateLEDs(ExhibitState::State state) {
        digitalWrite(PIN_LED_DECEL,    LOW);
        digitalWrite(PIN_LED_RECOVERY, LOW);
        digitalWrite(PIN_LED_STABLE,   LOW);
        switch (state) {
            case ExhibitState::NORMAL:
                digitalWrite(PIN_LED_STABLE, HIGH);
                break;
            case ExhibitState::DECELERATION:
                digitalWrite(PIN_LED_DECEL, HIGH);
                break;
            case ExhibitState::RECOVERY:
                digitalWrite(PIN_LED_RECOVERY, HIGH);
                break;
        }
    }

    void cycleAnalogClocks() {
        //Run on every loop. Controls advance of clocks to targets by switching pins per pulse width and max tick rate.
        //TODO if delays of over 60sec occur, it will sync up, but a minute behind. Solution: track more than secs?
        //clock 0 is pins 0 (even) and 1 (odd); 1 is 2 and 3; 2 is 4 and 5. Thus pin is i*2+(sec%2)
        for(int i=0; i<3; i++) {
            //If we are in the middle of a tick pulse, turn it off once it's been long enough
            if(analogClockEnergized[i]) { //Figure this is faster than checking the pin state via expander
                if(millis()-analogClockLastTick[i] > ANALOG_PULSE_WIDTH) {
                    analogClocks.write(i*2+(analogClockCurrent[i]%2), LOW);
                    analogClockEnergized[i] = false;
                }
            }
            //If we need to tick, start a tick, as long as the previous tick was long enough ago
            else if(analogClockCurrent[i] != analogClockTarget[i]) {
                if(millis()-analogClockLastTick[i] > ANALOG_MAX_TICK_RATE) {
                    analogClockCurrent[i]++; if(analogClockCurrent[i]>=60) analogClockCurrent[i]=0;
                    analogClockLastTick[i] = millis();
                    analogClocks.write(i*2+(analogClockCurrent[i]%2), HIGH);
                    analogClockEnergized[i] = true;
                }
            }
        }
        //The digital clocks do not need a similar function, as the TM1637 handles this.
    }
    
    void testPattern() {
        
        // All on, all eights, needle rise
        // All LEDs on [wip]
        // for (int i = 0; i < 3; i++) {
        //     digitalWrite(ledPins[i], HIGH);
        // }
        analogWrite(meterPin, 255); //TODO need to make the needle move gently
        for (int i = 0; i < 4; i++) {
            digitalClocks[i]->showString("888888");
        }
        delay(1000); //fix this
        
        // Fall, with display IDs
        // for (int i = 0; i < 3; i++) { [wip]
        //     digitalWrite(ledPins[i], LOW);
        // }
        analogWrite(meterPin, 128); //TODO need to make the needle move gently
        digitalClocks[0]->showString("OUT");
        digitalClocks[1]->showString("CHA");
        digitalClocks[2]->showString("DIF");
        digitalClocks[3]->showString("ELP");
        delay(1000); //fix this
        
        // Clear everything
        for (int i = 0; i < 4; i++) {
            digitalClocks[i]->clear();
        }

        analogWrite(meterPin, 0);
    }
    
private:
    void updateDigitalClock(int i, int32_t dec, bool colons) {
        //Sends update to TM1637 when display has changed
        if(digitalClockCurrent[i] != dec || (i<2 && digitalClockCurrentBlink[i] != colons)) {
            digitalClockCurrent[i] != dec;
            if(i<2) digitalClockCurrentBlink[i] != colons;
            digitalClocks[i].showNumberDec(
                digitalClockCurrent[i],
                ((i<2? 1-digitalClockCurrentBlink[i] : colons)? 0b00001010 : 0b00000000),
                true, 6
            );
        }
    }

};

#endif // DISPLAY_MANAGER_H