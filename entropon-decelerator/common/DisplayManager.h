// ==========================================
// DisplayManager.h - Shared display code
// ==========================================

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <PCF8574.h>
#include <TM1637TinyDisplay6.h>  // For 6-digit displays
#include "TimeTypes.h"

// Custom TM1637 class that works with PCF8574
class TM1637_PCF8574 {
private:
    PCF8574* expander;
    uint8_t clkPin;
    uint8_t dioPin;
    uint8_t brightness;
    uint8_t displayIndex;
    uint8_t render;
    
    // TM1637 commands
    static const uint8_t TM1637_CMD1 = 0x40;
    static const uint8_t TM1637_CMD2 = 0xC0;
    static const uint8_t TM1637_CMD3 = 0x80;

    // Brightness levels for each display
    static const uint8_t displayBrightness[4];
    
    // Digit to segment mapping
    static const uint8_t digitToSegment[];
    
    // Timing delays (microseconds)
    static const uint16_t TM1637_DELAY = 0;
    
    void start() {
        expander->write(dioPin, HIGH);
        expander->write(clkPin, HIGH);
        delayMicroseconds(TM1637_DELAY);
        expander->write(dioPin, LOW);
        expander->write(clkPin, LOW);
        delayMicroseconds(TM1637_DELAY);
    }
    
    void stop() {
        expander->write(clkPin, LOW);
        expander->write(dioPin, LOW);
        delayMicroseconds(TM1637_DELAY);
        expander->write(clkPin, HIGH);
        expander->write(dioPin, HIGH);
        delayMicroseconds(TM1637_DELAY);
    }
    
    bool writeByte(uint8_t data) {
        // Write 8 bits
        for (uint8_t i = 0; i < 8; i++) {
            expander->write(clkPin, LOW);
            delayMicroseconds(TM1637_DELAY);
            
            expander->write(dioPin, (data & 0x01) ? HIGH : LOW);
            data >>= 1;
            
            expander->write(clkPin, HIGH);
            delayMicroseconds(TM1637_DELAY);
        }
        
        // Wait for ACK
        expander->write(clkPin, LOW);
        expander->write(dioPin, HIGH);
        delayMicroseconds(TM1637_DELAY);
        
        expander->write(clkPin, HIGH);
        delayMicroseconds(TM1637_DELAY);
        
        bool ack = (expander->read(dioPin) == LOW);
        
        expander->write(clkPin, LOW);
        delayMicroseconds(TM1637_DELAY);
        
        return ack;
    }
    
public:
    TM1637_PCF8574(PCF8574* exp, uint8_t clk, uint8_t dio, uint8_t index = 0) 
        : expander(exp), clkPin(clk), dioPin(dio), displayIndex(index) {
        brightness = displayBrightness[index];
    }
    
    void begin() {
        // Set pins as outputs
        expander->write(clkPin, HIGH);
        expander->write(dioPin, HIGH);
    }
    
    void setBrightness(uint8_t bright) {
        brightness = bright & 0x0f;
    }
    
    void clear() {
        uint8_t data[6] = {0, 0, 0, 0, 0, 0};
        setSegments(data);
    }
    
    void setSegments(const uint8_t segments[], uint8_t length = 6) {
        // Reverse the segment order to correct for backwards hardware
        uint8_t reversedSegments[6];
        for (uint8_t i = 0; i < 6 && i < length; i++) {
            // Swap first 3 with last 3
            if (i < 3) {
                reversedSegments[i] = (length > i + 3) ? segments[i + 3] : 0;
            } else {
                reversedSegments[i] = segments[i - 3];
            }
        }

        // Write COMM1
        start();
        writeByte(TM1637_CMD1);
        stop();
        
        // Write COMM2 + first digit address
        start();
        writeByte(TM1637_CMD2);
        
        // Write the data bytes
        for (uint8_t i = 0; i < length && i < 6; i++) {
            // if(i<3) writeByte(segments[2-i]);
            // else    writeByte(segments[8-i]);
            writeByte(segments[i]);
        }
        stop();
        
        // Write COMM3 + brightness
        start();
        writeByte(TM1637_CMD3 | 0x08 | brightness);
        stop();
    }
    
    void showNumberDec(uint32_t num, uint8_t dots = 0, bool leadingZeros = true) {
        uint8_t segments[6];
        
        // Convert number to segments
        for (int8_t i = 5; i >= 0; i--) {

            if(render==20) Serial.print("i=");
            if(render==20) Serial.print(i,DEC);

            uint8_t pos = (i<3? 2-i: 8-i); //345012, left to right

            if(render==20) Serial.print(" pos=");
            if(render==20) Serial.print(pos,DEC);

            uint8_t digit = num % 10;

            if(render==20) Serial.print(" digit=");
            if(render==20) Serial.print(digit,DEC);

            segments[pos] = digitToSegment[digit];
            num /= 10;
            
            // Add dots if specified
            if(render==20) Serial.print(" dot=");
            if(render==20) Serial.print((dots & (1 << i)? "y": "n"));

            if(render==20) Serial.print(" segPre=");
            if(render==20) Serial.print(segments[pos],DEC);


            if (dots & (1 << i)) {
                segments[pos] |= 0x80;
            }

            if(render==20) Serial.print(" segPost=");
            if(render==20) Serial.print(segments[pos],DEC);
            
            // Handle leading zeros
            if (!leadingZeros && num == 0 && i > 0) {
                bool allZero = true;
                for (int8_t j = i - 1; j >= 0; j--) {
                    if ((segments[(j<3? 2-j: 8-j)] & 0x7F) != digitToSegment[0]) {
                        allZero = false;
                        break;
                    }
                }
                if (allZero) {
                    segments[(i<3? 2-i: 8-i)-1] = 0;  // Clear leading zero
                }
            }

            if(render==20) Serial.println();
        }
        
        setSegments(segments);
        // render++;
    }
    
    void showString(const char* str) {
        uint8_t segments[6] = {0};
        uint8_t pos = 0;
        
        for (uint8_t i = 0; str[i] != '\0' && pos < 6; i++) {
            if (str[i] >= '0' && str[i] <= '9') {
                segments[pos++] = digitToSegment[str[i] - '0'];
            } else if (str[i] == '-') {
                segments[pos++] = 0x40;  // Minus sign
            } else if (str[i] == ' ') {
                segments[pos++] = 0x00;  // Blank
            } else if (str[i] == ':' && pos > 0) {
                segments[pos-1] |= 0x80;  // Add colon to previous digit
            } else if (str[i] == '.' && pos > 0) {
                segments[pos-1] |= 0x80;  // Add dot to previous digit
            }
        }
        
        setSegments(segments);
    }
};

// Segment definitions for digits 0-9
// const uint8_t TM1637_PCF8574::digitToSegment[] = {
//     0x3F,  // 0
//     0x06,  // 1
//     0x5B,  // 2
//     0x4F,  // 3
//     0x66,  // 4
//     0x6D,  // 5
//     0x7D,  // 6
//     0x07,  // 7
//     0x7F,  // 8
//     0x6F   // 9
// };
const uint8_t TM1637_PCF8574::digitToSegment[] = {
//    .GFEDCBA
    0b00111111,  // 0
    0b00000110,  // 1
    0b01011011,  // 2
    0b01001111,  // 3
    0b01100110,  // 4.
    0b01101101,  // 5
    0b01111100,  // 6
    0b00000111,  // 7
    0b01111111,  // 8
    0b01100111   // 9
};

// Brightness levels for each display
const uint8_t TM1637_PCF8574::displayBrightness[4] = {
    0x0F,  // Display 0
    0x0A,  // Display 1
    0x0C,  // Display 2
    0x01   // Display 3
};

class DisplayManager {
private:
    // PCF8574 expander for displays
    PCF8574* displayExpander;
    
    // Display objects using custom PCF8574 wrapper
    TM1637_PCF8574* digitalClocks[4];

    // Pin assignments
    uint8_t ledPins[3];
    uint8_t meterPin;
    uint8_t analogClockPins[6];  // 3 clocks × 2 pins each
    
    // Tracking for clock pulses
    uint8_t analogClockLastSeconds[3];
    bool analogClockOddPinPulse[3];
    
    // Display freeze state for recovery mode
    bool freezeDisplays;

    // //WIP
    // uint32_t timeLast;
    // bool blinkStateLast;
    
public:
    // Constructor
    DisplayManager(
        uint8_t expanderAddress,
        // uint8_t ledNormal, uint8_t ledDecel, uint8_t ledRecovery, [wip]
        uint8_t meterPWMPin //,
        // uint8_t analogClockOutsideEvenPin, uint8_t analogClockOutsideOddPin, [wip]
        // uint8_t analogClockChamberEvenPin, uint8_t analogClockChamberOddPin,
        // uint8_t analogClockSavedEvenPin, uint8_t analogClockSavedOddPin
    ) : meterPin(meterPWMPin), freezeDisplays(false) {
        // Create PCF8574 expander
        displayExpander = new PCF8574(expanderAddress);
        
        // Initialize displays on PCF8574
        // Display 1: CLK=P7, DIO=P6
        // Display 2: CLK=P5, DIO=P4
        // Display 3: CLK=P3, DIO=P2
        // Display 4: CLK=P1, DIO=P0
        digitalClocks[0] = new TM1637_PCF8574(displayExpander, 6, 7, 0);
        digitalClocks[1] = new TM1637_PCF8574(displayExpander, 4, 5, 1);
        digitalClocks[2] = new TM1637_PCF8574(displayExpander, 2, 3, 2);
        digitalClocks[3] = new TM1637_PCF8574(displayExpander, 0, 1, 3);

        // Store LED pins [wip]
        // ledPins[0] = ledNormal;
        // ledPins[1] = ledDecel;
        // ledPins[2] = ledRecovery;
        
        // Store clock pins [wip]
        // analogClockPins[0] = analogClockOutsideEvenPin;
        // analogClockPins[1] = analogClockOutsideOddPin;
        // analogClockPins[2] = analogClockChamberEvenPin;
        // analogClockPins[3] = analogClockChamberOddPin;
        // analogClockPins[4] = analogClockSavedEvenPin;
        // analogClockPins[5] = analogClockSavedOddPin;
        
        // Initialize tracking
        for (int i = 0; i < 3; i++) {
            analogClockLastSeconds[i] = 255;  // Invalid value to force first pulse
            analogClockOddPinPulse[i] = false;
        }
    }
    
    // Initialize all displays and outputs
    void begin() {
        // Initialize I2C and PCF8574
        Wire.begin();
        displayExpander->begin();
        
        // Configure all PCF8574 pins as outputs (HIGH = output for this library)
        for (uint8_t i = 0; i < 8; i++) {
            displayExpander->write(i, HIGH);
        }
        
        // Initialize displays
        for (int i = 0; i < 4; i++) {
            digitalClocks[i]->begin();
            // digitalClocks[i]->setBrightness(0x0f);  // Max brightness
            digitalClocks[i]->clear();
        }
        
        // Set up LED pins [wip]
        // for (int i = 0; i < 3; i++) {
        //     pinMode(ledPins[i], OUTPUT);
        //     digitalWrite(ledPins[i], LOW);
        // }
        
        // Set up meter pin
        pinMode(meterPin, OUTPUT);
        analogWrite(meterPin, 0);
        
        // Set up clock pins [wip]
        // for (int i = 0; i < 6; i++) {
        //     pinMode(analogClockPins[i], OUTPUT);
        //     digitalWrite(analogClockPins[i], LOW);
        // }
    }
    
    // Update time display (display 0 or 1)
    void updateDigitalClock(uint8_t displayNum, const TimeValue& time) {
        if (displayNum > 1) return;
        digitalClocks[displayNum]->showNumberDec(
            time.getHHMMSS(),
            (1-(time.getMils()/500) ? 0b00001010 : 0b00000000)//, //show separators on first half of second
            // true, 6 //show leading zeroes on six digits
        );
        
        // Handle clock pulse for this display [wip]
        // if (displayNum == 0) {  // Outside clock
        //     updateAnalogClock(0, seconds);
        // } else {  // Chamber clock
        //     updateAnalogClock(1, seconds);
        // }
    }
    
    // Update difference display (display 2)
    void updateDigitalClockDifference(const TimeValue& outside, const TimeValue& chamber) {
        // Check if we should freeze the display
        if (freezeDisplays) return;
        
        // Calculate difference in milliseconds
        int32_t diffMillis = outside.getDifferenceMillis(chamber);
        // bool negative = (diffMillis < 0);
        // if (negative) diffMillis = -diffMillis;
        
        // Convert to minutes, seconds, hundredths
        uint16_t totalSeconds = diffMillis / 1000;
        uint8_t minutes = totalSeconds / 60;
        uint8_t seconds = totalSeconds % 60;
        uint8_t hundredths = (diffMillis % 1000) / 10;
        
        // Format: MM:SS.HH (or -MM:SS.HH for negative)
        // if (negative) {
        //     // Show negative sign with the time
        //     // Using showString for custom formatting
        //     char timeStr[8];
        //     sprintf(timeStr, "-%02d%02d%02d", minutes, seconds, hundredths);
        //     digitalClocks[2]->showString(timeStr);
        // } else {
            // Show positive time with dots
            // Create the number as MMSSSS (where last two are hundredths)
            uint32_t displayValue = minutes * 10000L + seconds * 100L + hundredths;
            
            // Dots at position 1 (after MM) and position 3 (after SS)
            uint8_t dots = 0b00001010;  // Colon after position 1, decimal after position 3
            digitalClocks[2]->showNumberDec(displayValue, dots, true);
        // }
        
        // Update saved time clock (only for positive differences)
        // if (!negative) { [wip]
        //     updateAnalogClockSaved(totalSeconds);
        // }
    }
    
    // Update elapsed time display (display 3)
    void updateDigitalClockElapsed(uint32_t elapsedMillis) {
        // Check if we should freeze the display
        if (freezeDisplays) return;
        
        // Convert to hours, minutes, seconds
        uint32_t totalSeconds = elapsedMillis / 1000;
        uint8_t hours = (totalSeconds / 3600) % 100;  // Max 99 hours
        uint8_t minutes = (totalSeconds / 60) % 60;
        uint8_t seconds = totalSeconds % 60;
        
        // Format: HH:MM:SS
        uint32_t displayValue = hours * 10000L + minutes * 100L + seconds;
        
        // Fixed colons at positions 1 and 3
        uint8_t dots = 0b00001010;
        digitalClocks[3]->showNumberDec(displayValue, dots, true);
    }
    
    // Freeze or unfreeze difference and elapsed displays
    void setDisplayFreeze(bool freeze) {
        freezeDisplays = freeze;
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
    
    // Update state LEDs
    void updateLEDs(ExhibitState::State state) {
        // Turn off all LEDs first
        // for (int i = 0; i < 3; i++) { [wip]
        //     digitalWrite(ledPins[i], LOW);
        // }
        
        // Turn on appropriate LED [wip]
        // switch (state) {
        //     case ExhibitState::NORMAL:
        //         digitalWrite(ledPins[0], HIGH);
        //         break;
        //     case ExhibitState::DECELERATION:
        //         digitalWrite(ledPins[1], HIGH);
        //         break;
        //     case ExhibitState::RECOVERY:
        //         digitalWrite(ledPins[2], HIGH);
        //         break;
        // }
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
        // digitalClocks[0]->showString("OUT");
        // digitalClocks[1]->showString("CHA");
        // digitalClocks[2]->showString("DIF");
        // digitalClocks[3]->showString("ELP");
        delay(1000); //fix this
        
        // Clear everything
        for (int i = 0; i < 4; i++) {
            digitalClocks[i]->clear();
        }        
        
        analogWrite(meterPin, 0);
    }
    
private:
    // Update Outside and Chamber analog clocks
    void updateAnalogClock(uint8_t clockIndex, uint8_t currentSecond) {
        // if (clockIndex > 2) return; [wip]
        
        // // Check if second has changed
        // if (currentSecond != analogClockLastSeconds[clockIndex]) {
        //     analogClockLastSeconds[clockIndex] = currentSecond;
            
        //     // Determine which pin to pulse
        //     uint8_t pinIndex = clockIndex * 2;
        //     if (analogClockOddPinPulse[clockIndex]) {
        //         pinIndex++;  // Use OddPin pin
        //     }
            
        //     // Send pulse (30ms duration per Lavet motor spec)
        //     digitalWrite(analogClockPins[pinIndex], HIGH);
        //     delay(30);
        //     digitalWrite(analogClockPins[pinIndex], LOW);
            
        //     // Toggle OddPin/EvenPin state
        //     analogClockOddPinPulse[clockIndex] = !analogClockOddPinPulse[clockIndex];
        // }
    }
    
    // Update Timed Saved analog clock
    void updateAnalogClockSaved(uint32_t totalSeconds) {
        // static uint32_t lastSavedSeconds = 0;
        
        // // Check if we've gained a new second
        // if (totalSeconds > lastSavedSeconds) {
        //     uint32_t newSeconds = totalSeconds - lastSavedSeconds;
        //     lastSavedSeconds = totalSeconds;
            
        //     // Pulse for each new second
        //     for (uint32_t i = 0; i < newSeconds; i++) {
        //         uint8_t pinIndex = 4;  // Saved clock EvenPin pin
        //         if (analogClockOddPinPulse[2]) {
        //             pinIndex = 5;  // Saved clock OddPin pin
        //         }
                
        //         digitalWrite(analogClockPins[pinIndex], HIGH);
        //         delay(30); //fix this
        //         digitalWrite(analogClockPins[pinIndex], LOW);
                
        //         analogClockOddPinPulse[2] = !analogClockOddPinPulse[2];
                
        //         // Small delay between pulses if multiple
        //         if (i < newSeconds - 1) {
        //             //delay(50); //fix this
        //         }
        //     }
        // }
    }
};

#endif // DISPLAY_MANAGER_H