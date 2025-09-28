// ==========================================
// DisplayManager.h - Shared display code
// ==========================================

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <TM1637TinyDisplay6.h>  // For 6-digit displays
#include "TimeTypes.h"

class DisplayManager {
private:
    // Display objects (must be initialized with pins in constructor)
    TM1637TinyDisplay6* displays[4];
    
    // Pin assignments
    uint8_t ledPins[3];
    uint8_t meterPin;
    uint8_t clockPins[6];  // 3 clocks × 2 pins each
    
    // Tracking for clock pulses
    uint8_t lastSeconds[3];
    bool clockOddPulse[3];
    
    // Display blink state
    bool blinkState;
    uint32_t lastBlinkMillis;
    
    // Display freeze state for recovery mode
    bool freezeDisplays;
    
public:
    // Constructor
    DisplayManager(
        uint8_t display1CLK, uint8_t display1DIO,
        uint8_t display2CLK, uint8_t display2DIO,
        uint8_t display3CLK, uint8_t display3DIO,
        uint8_t display4CLK, uint8_t display4DIO,
        uint8_t ledNormal, uint8_t ledDecel, uint8_t ledRecovery,
        uint8_t meterPWM,
        uint8_t clockOutEven, uint8_t clockOutOdd,
        uint8_t clockChamberEven, uint8_t clockChamberOdd,
        uint8_t clockSavedEven, uint8_t clockSavedOdd
    ) : meterPin(meterPWM), freezeDisplays(false) {
        // Initialize displays using TM1637TinyDisplay6
        displays[0] = new TM1637TinyDisplay6(display1CLK, display1DIO);
        displays[1] = new TM1637TinyDisplay6(display2CLK, display2DIO);
        displays[2] = new TM1637TinyDisplay6(display3CLK, display3DIO);
        displays[3] = new TM1637TinyDisplay6(display4CLK, display4DIO);
        
        // Store LED pins
        ledPins[0] = ledNormal;
        ledPins[1] = ledDecel;
        ledPins[2] = ledRecovery;
        
        // Store clock pins
        clockPins[0] = clockOutEven;
        clockPins[1] = clockOutOdd;
        clockPins[2] = clockChamberEven;
        clockPins[3] = clockChamberOdd;
        clockPins[4] = clockSavedEven;
        clockPins[5] = clockSavedOdd;
        
        // Initialize tracking
        for (int i = 0; i < 3; i++) {
            lastSeconds[i] = 255;  // Invalid value to force first pulse
            clockOddPulse[i] = false;
        }
        
        blinkState = false;
        lastBlinkMillis = 0;
    }
    
    // Initialize all displays and outputs
    void begin() {
        // Set up displays
        for (int i = 0; i < 4; i++) {
            displays[i]->begin();
            displays[i]->setBrightness(BRIGHT_HIGH);  // Max brightness
            displays[i]->clear();
        }
        
        // Set up LED pins
        for (int i = 0; i < 3; i++) {
            pinMode(ledPins[i], OUTPUT);
            digitalWrite(ledPins[i], LOW);
        }
        
        // Set up meter pin
        pinMode(meterPin, OUTPUT);
        analogWrite(meterPin, 0);
        
        // Set up clock pins
        for (int i = 0; i < 6; i++) {
            pinMode(clockPins[i], OUTPUT);
            digitalWrite(clockPins[i], LOW);
        }
    }
    
    // Update time display (display 0 or 1)
    void updateTimeDisplay(uint8_t displayNum, const TimeValue& time) {
        if (displayNum > 1) return;
        
        // Update blink state (for colon)
        uint32_t currentMillis = millis();
        if (currentMillis - lastBlinkMillis > 500) {
            blinkState = !blinkState;
            lastBlinkMillis = currentMillis;
        }
        
        // Format: HH:MM:SS
        uint8_t hours = time.getHours();
        uint8_t minutes = time.getMinutes();
        uint8_t seconds = time.getSeconds();
        
        // Create display buffer
        uint8_t buffer[6];
        buffer[0] = hours / 10;
        buffer[1] = hours % 10;
        buffer[2] = minutes / 10;
        buffer[3] = minutes % 10;
        buffer[4] = seconds / 10;
        buffer[5] = seconds % 10;
        
        // Show with or without dots based on blink state
        if (blinkState) {
            // Show time with dots between HH:MM:SS
            displays[displayNum]->showNumberDec(hours, 0b01000000, true, 2, 0);  // HH with colon
            displays[displayNum]->showNumberDec(minutes, 0b01000000, true, 2, 2); // MM with colon
            displays[displayNum]->showNumberDec(seconds, 0, true, 2, 4);          // SS
        } else {
            // Show time without dots
            displays[displayNum]->showNumberDec(hours * 10000L + minutes * 100L + seconds, 0b00000000, true);
        }
        
        // Handle clock pulse for this display
        if (displayNum == 0) {  // Outside clock
            updateClockPulse(0, seconds);
        } else {  // Chamber clock
            updateClockPulse(1, seconds);
        }
    }
    
    // Alternative method using showTime for cleaner time display
    void updateTimeDisplayAlt(uint8_t displayNum, const TimeValue& time) {
        if (displayNum > 1) return;
        
        uint8_t hours = time.getHours();
        uint8_t minutes = time.getMinutes();
        uint8_t seconds = time.getSeconds();
        
        // TM1637TinyDisplay6 can show time in HH:MM:SS format directly
        // Create time value as HHMMSS
        uint32_t timeValue = hours * 10000L + minutes * 100L + seconds;
        
        // Update blink state
        uint32_t currentMillis = millis();
        if (currentMillis - lastBlinkMillis > 500) {
            blinkState = !blinkState;
            lastBlinkMillis = currentMillis;
        }
        
        // Show with blinking colons
        uint8_t dots = blinkState ? 0b01010000 : 0b00000000;  // Dots at positions 1 and 3
        displays[displayNum]->showNumberDec(timeValue, dots, true);
        
        // Handle clock pulse
        if (displayNum == 0) {
            updateClockPulse(0, seconds);
        } else {
            updateClockPulse(1, seconds);
        }
    }
    
    // Update difference display (display 2)
    void updateDifferenceDisplay(const TimeValue& outside, const TimeValue& chamber) {
        // Check if we should freeze the display
        if (freezeDisplays) return;
        
        // Calculate difference in milliseconds
        int32_t diffMillis = chamber.getDifferenceMillis(outside);
        bool negative = (diffMillis < 0);
        if (negative) diffMillis = -diffMillis;
        
        // Convert to minutes, seconds, hundredths
        uint16_t totalSeconds = diffMillis / 1000;
        uint8_t minutes = totalSeconds / 60;
        uint8_t seconds = totalSeconds % 60;
        uint8_t hundredths = (diffMillis % 1000) / 10;
        
        // Format: MM:SS.HH (or -MM:SS.HH for negative)
        if (negative) {
            // Show negative sign with the time
            // Using showString for custom formatting
            char timeStr[8];
            sprintf(timeStr, "-%02d%02d%02d", minutes, seconds, hundredths);
            displays[2]->showString(timeStr);
        } else {
            // Show positive time with dots
            // Create the number as MMSSSS (where last two are hundredths)
            uint32_t displayValue = minutes * 10000L + seconds * 100L + hundredths;
            
            // Dots at position 1 (after MM) and position 3 (after SS)
            uint8_t dots = 0b01010000;  // Colon after position 1, decimal after position 3
            displays[2]->showNumberDec(displayValue, dots, true);
        }
        
        // Update saved time clock (only for positive differences)
        if (!negative) {
            updateSavedClock(totalSeconds);
        }
    }
    
    // Update elapsed time display (display 3)
    void updateElapsedDisplay(uint32_t elapsedMillis) {
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
        uint8_t dots = 0b01010000;
        displays[3]->showNumberDec(displayValue, dots, true);
    }
    
    // Freeze or unfreeze difference and elapsed displays
    void setDisplayFreeze(bool freeze) {
        freezeDisplays = freeze;
    }
    
    // Update analog meter with PWM
    void updateMeter(int32_t rateQ16) {
        // Convert Q16 rate to seconds lost per second
        // Rate of 1.0 (Q16_ONE) = 0 seconds lost
        // Rate of 0.0 = 1 second lost per second
        float rate = (float)rateQ16 / (float)Q16_ONE;
        float secondsLost = 1.0f - rate;
        
        // Clamp to 0-1 range
        if (secondsLost < 0.0f) secondsLost = 0.0f;
        if (secondsLost > 1.0f) secondsLost = 1.0f;
        
        // Convert to PWM value (0-255)
        uint8_t pwmValue = (uint8_t)(secondsLost * 255.0f);
        analogWrite(meterPin, pwmValue);
    }
    
    // Update state LEDs
    void updateLEDs(ExhibitState::State state) {
        // Turn off all LEDs first
        for (int i = 0; i < 3; i++) {
            digitalWrite(ledPins[i], LOW);
        }
        
        // Turn on appropriate LED
        switch (state) {
            case ExhibitState::NORMAL:
                digitalWrite(ledPins[0], HIGH);
                break;
            case ExhibitState::DECELERATION:
                digitalWrite(ledPins[1], HIGH);
                break;
            case ExhibitState::RECOVERY:
                digitalWrite(ledPins[2], HIGH);
                break;
        }
    }
    
    // Test all displays using TM1637TinyDisplay6 animations
    void testPattern() {
        // Use built-in test features of TM1637TinyDisplay6
        const uint8_t allOn[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        const uint8_t message[] = {
            SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,           // H
            SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,           // E
            SEG_D | SEG_E | SEG_F,                           // L
            SEG_D | SEG_E | SEG_F,                           // L
            SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
            0x00                                              // Space
        };
        
        // Show "HELLO" on all displays
        for (int i = 0; i < 4; i++) {
            displays[i]->setSegments(message);
        }
        
        // All LEDs on
        for (int i = 0; i < 3; i++) {
            digitalWrite(ledPins[i], HIGH);
        }
        
        // Full meter
        analogWrite(meterPin, 255);
        
        delay(1000);
        
        // Animate with scrolling text
        for (int i = 0; i < 4; i++) {
            displays[i]->showString("888888");
        }
        delay(500);
        
        // Use built-in animation if available
        for (int i = 0; i < 4; i++) {
            displays[i]->showAnimation(ANIMATION_FRAME_IN, ANIMATION_FRAME_OUT, 500);
        }
        
        // Clear everything
        for (int i = 0; i < 4; i++) {
            displays[i]->clear();
        }
        
        for (int i = 0; i < 3; i++) {
            digitalWrite(ledPins[i], LOW);
        }
        
        analogWrite(meterPin, 0);
    }
    
    // Utility method to show custom text on any display
    void showText(uint8_t displayNum, const char* text) {
        if (displayNum > 3) return;
        displays[displayNum]->showString(text);
    }
    
    // Utility method to show scrolling text
    void scrollText(uint8_t displayNum, const char* text, uint16_t scrollDelay = 200) {
        if (displayNum > 3) return;
        displays[displayNum]->showString_P(text, scrollDelay);
    }
    
private:
    // Update clock pulse for a specific clock
    void updateClockPulse(uint8_t clockIndex, uint8_t currentSecond) {
        if (clockIndex > 2) return;
        
        // Check if second has changed
        if (currentSecond != lastSeconds[clockIndex]) {
            lastSeconds[clockIndex] = currentSecond;
            
            // Determine which pin to pulse
            uint8_t pinIndex = clockIndex * 2;
            if (clockOddPulse[clockIndex]) {
                pinIndex++;  // Use odd pin
            }
            
            // Send pulse (30ms duration per Lavet motor spec)
            digitalWrite(clockPins[pinIndex], HIGH);
            delay(30);
            digitalWrite(clockPins[pinIndex], LOW);
            
            // Toggle odd/even state
            clockOddPulse[clockIndex] = !clockOddPulse[clockIndex];
        }
    }
    
    // Update saved time clock
    void updateSavedClock(uint32_t totalSeconds) {
        static uint32_t lastSavedSeconds = 0;
        
        // Check if we've gained a new second
        if (totalSeconds > lastSavedSeconds) {
            uint32_t newSeconds = totalSeconds - lastSavedSeconds;
            lastSavedSeconds = totalSeconds;
            
            // Pulse for each new second
            for (uint32_t i = 0; i < newSeconds; i++) {
                uint8_t pinIndex = 4;  // Saved clock even pin
                if (clockOddPulse[2]) {
                    pinIndex = 5;  // Saved clock odd pin
                }
                
                digitalWrite(clockPins[pinIndex], HIGH);
                delay(30);
                digitalWrite(clockPins[pinIndex], LOW);
                
                clockOddPulse[2] = !clockOddPulse[2];
                
                // Small delay between pulses if multiple
                if (i < newSeconds - 1) {
                    delay(50);
                }
            }
        }
    }
};

#endif // DISPLAY_MANAGER_H