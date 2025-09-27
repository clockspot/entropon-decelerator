// ==========================================
// TimeTypes.h - Common data structures
// ==========================================

#ifndef TIME_TYPES_H
#define TIME_TYPES_H

#include <Arduino.h>

// Constants
#define MILLIS_PER_DAY 86400000UL
#define MILLIS_PER_HOUR 3600000UL
#define MILLIS_PER_MINUTE 60000UL
#define MILLIS_PER_SECOND 1000UL

// Fixed-point Q16 constants
#define Q16_ONE 65536L
#define Q16_HALF 32768L

struct TimeValue {
    uint32_t millisSinceMidnight;
    
    // Add milliseconds and handle rollover
    void addMillis(uint32_t delta) {
        millisSinceMidnight += delta;
        normalize();
    }
    
    // Handle midnight rollover
    void normalize() {
        while (millisSinceMidnight >= MILLIS_PER_DAY) {
            millisSinceMidnight -= MILLIS_PER_DAY;
        }
    }
    
    // Get time components
    uint8_t getHours() const {
        return (millisSinceMidnight / MILLIS_PER_HOUR) % 24;
    }
    
    uint8_t getMinutes() const {
        return (millisSinceMidnight / MILLIS_PER_MINUTE) % 60;
    }
    
    uint8_t getSeconds() const {
        return (millisSinceMidnight / MILLIS_PER_SECOND) % 60;
    }
    
    uint16_t getMillis() const {
        return millisSinceMidnight % MILLIS_PER_SECOND;
    }
    
    // Get total seconds (for display purposes)
    uint32_t getTotalSeconds() const {
        return millisSinceMidnight / MILLIS_PER_SECOND;
    }
    
    // Calculate difference between two times (handling midnight wrap)
    int32_t getDifferenceMillis(const TimeValue& other) const {
        int32_t diff = (int32_t)millisSinceMidnight - (int32_t)other.millisSinceMidnight;
        
        // Handle midnight wrap-around
        if (diff > (int32_t)(MILLIS_PER_DAY / 2)) {
            diff -= MILLIS_PER_DAY;
        } else if (diff < -(int32_t)(MILLIS_PER_DAY / 2)) {
            diff += MILLIS_PER_DAY;
        }
        
        return diff;
    }
    
    // Set time from hours, minutes, seconds
    void setTime(uint8_t hours, uint8_t minutes, uint8_t seconds) {
        millisSinceMidnight = (uint32_t)hours * MILLIS_PER_HOUR +
                             (uint32_t)minutes * MILLIS_PER_MINUTE +
                             (uint32_t)seconds * MILLIS_PER_SECOND;
        normalize();
    }
};

struct ExhibitState {
    enum State { 
        NORMAL = 0, 
        DECELERATION = 1, 
        RECOVERY = 2 
    };
    
    State current;
    uint32_t elapsedMillis;
    int32_t chamberRateQ16;  // Fixed-point Q16 format (1.0 = 65536)
    
    // Initialize to default state
    void reset() {
        current = NORMAL;
        elapsedMillis = 0;
        chamberRateQ16 = Q16_ONE;
    }
    
    // Convert rate to human-readable float (for display)
    float getRateAsFloat() const {
        return (float)chamberRateQ16 / (float)Q16_ONE;
    }
    
    // Get seconds lost per second (for meter display)
    float getSecondsLostPerSecond() const {
        return 1.0f - getRateAsFloat();
    }
};

#endif // TIME_TYPES_H

// ==========================================
// DisplayManager.h - Shared display code
// ==========================================

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <TM1637Display.h>
#include "TimeTypes.h"

class DisplayManager {
private:
    // Display objects (must be initialized with pins in constructor)
    TM1637Display* displays[4];
    
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
    ) : meterPin(meterPWM) {
        // Initialize displays
        displays[0] = new TM1637Display(display1CLK, display1DIO);
        displays[1] = new TM1637Display(display2CLK, display2DIO);
        displays[2] = new TM1637Display(display3CLK, display3DIO);
        displays[3] = new TM1637Display(display4CLK, display4DIO);
        
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
            displays[i]->setBrightness(0x0f);  // Max brightness
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
        
        // Create display segments
        uint8_t segments[6];
        segments[0] = displays[displayNum]->encodeDigit(hours / 10);
        segments[1] = displays[displayNum]->encodeDigit(hours % 10);
        segments[2] = displays[displayNum]->encodeDigit(minutes / 10);
        segments[3] = displays[displayNum]->encodeDigit(minutes % 10);
        segments[4] = displays[displayNum]->encodeDigit(seconds / 10);
        segments[5] = displays[displayNum]->encodeDigit(seconds % 10);
        
        // Add blinking colons
        if (blinkState) {
            segments[1] |= 0x80;  // Colon after hours
            segments[3] |= 0x80;  // Colon after minutes
        }
        
        // Update display
        displays[displayNum]->setSegments(segments, 6);
        
        // Handle clock pulse for this display
        if (displayNum == 0) {  // Outside clock
            updateClockPulse(0, seconds);
        } else {  // Chamber clock
            updateClockPulse(1, seconds);
        }
    }
    
    // Update difference display (display 2)
    void updateDifferenceDisplay(const TimeValue& outside, const TimeValue& chamber) {
        // Calculate difference in milliseconds
        int32_t diffMillis = chamber.getDifferenceMillis(outside);
        bool negative = (diffMillis < 0);
        if (negative) diffMillis = -diffMillis;
        
        // Convert to minutes, seconds, hundredths
        uint16_t totalSeconds = diffMillis / 1000;
        uint8_t minutes = totalSeconds / 60;
        uint8_t seconds = totalSeconds % 60;
        uint8_t hundredths = (diffMillis % 1000) / 10;
        
        // Format: MM:SS.HH (or -MM:SS for negative)
        uint8_t segments[6];
        
        if (negative && minutes > 0) {
            // Show minus sign
            segments[0] = 0x40;  // Minus segment
            segments[1] = displays[2]->encodeDigit(minutes % 10);
        } else {
            segments[0] = displays[2]->encodeDigit(minutes / 10);
            segments[1] = displays[2]->encodeDigit(minutes % 10);
        }
        
        segments[2] = displays[2]->encodeDigit(seconds / 10);
        segments[3] = displays[2]->encodeDigit(seconds % 10);
        segments[4] = displays[2]->encodeDigit(hundredths / 10);
        segments[5] = displays[2]->encodeDigit(hundredths % 10);
        
        // Add fixed colons and decimal point
        segments[1] |= 0x80;  // Colon after minutes
        segments[3] |= 0x80;  // Decimal point after seconds
        
        displays[2]->setSegments(segments, 6);
        
        // Update saved time clock (only for positive differences)
        if (!negative) {
            updateSavedClock(totalSeconds);
        }
    }
    
    // Update elapsed time display (display 3)
    void updateElapsedDisplay(uint32_t elapsedMillis) {
        // Convert to hours, minutes, seconds
        uint32_t totalSeconds = elapsedMillis / 1000;
        uint8_t hours = (totalSeconds / 3600) % 100;  // Max 99 hours
        uint8_t minutes = (totalSeconds / 60) % 60;
        uint8_t seconds = totalSeconds % 60;
        
        // Format: HH:MM:SS
        uint8_t segments[6];
        segments[0] = displays[3]->encodeDigit(hours / 10);
        segments[1] = displays[3]->encodeDigit(hours % 10);
        segments[2] = displays[3]->encodeDigit(minutes / 10);
        segments[3] = displays[3]->encodeDigit(minutes % 10);
        segments[4] = displays[3]->encodeDigit(seconds / 10);
        segments[5] = displays[3]->encodeDigit(seconds % 10);
        
        // Add fixed colons
        segments[1] |= 0x80;  // Colon after hours
        segments[3] |= 0x80;  // Colon after minutes
        
        displays[3]->setSegments(segments, 6);
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
    
    // Test all displays
    void testPattern() {
        // All segments on
        uint8_t allOn[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        
        for (int i = 0; i < 4; i++) {
            displays[i]->setSegments(allOn);
        }
        
        // All LEDs on
        for (int i = 0; i < 3; i++) {
            digitalWrite(ledPins[i], HIGH);
        }
        
        // Full meter
        analogWrite(meterPin, 255);
        
        delay(1000);
        
        // Clear everything
        for (int i = 0; i < 4; i++) {
            displays[i]->clear();
        }
        
        for (int i = 0; i < 3; i++) {
            digitalWrite(ledPins[i], LOW);
        }
        
        analogWrite(meterPin, 0);
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

// ==========================================
// SerialProtocol.h - Communication protocol
// ==========================================

#ifndef SERIAL_PROTOCOL_H
#define SERIAL_PROTOCOL_H

#include <Arduino.h>
#include "TimeTypes.h"

class SerialProtocol {
public:
    // Message types
    static const uint8_t MSG_STATE = 0x01;
    static const uint8_t MSG_TIME = 0x02;
    static const uint8_t MSG_STOP = 0x03;
    static const uint8_t MSG_ACK = 0x04;
    static const uint8_t MSG_ERROR = 0xFF;
    
    // Protocol constants
    static const uint8_t START_BYTE = 0xAA;
    static const uint8_t END_BYTE = 0x55;
    static const uint16_t MAX_PAYLOAD_SIZE = 64;
    
private:
    Stream* serial;
    uint8_t receiveBuffer[MAX_PAYLOAD_SIZE + 5];  // Start + Type + Length + Payload + Checksum + End
    uint16_t receiveIndex;
    uint32_t lastReceiveTime;
    
public:
    // Constructor
    SerialProtocol(Stream* serialPort) : serial(serialPort), receiveIndex(0), lastReceiveTime(0) {}
    
    // Send state update
    void sendStateUpdate(const ExhibitState& state) {
        uint8_t payload[sizeof(ExhibitState)];
        memcpy(payload, &state, sizeof(ExhibitState));
        sendMessage(MSG_STATE, payload, sizeof(ExhibitState));
    }
    
    // Send time update
    void sendTimeUpdate(const TimeValue& outside, const TimeValue& chamber) {
        uint8_t payload[sizeof(TimeValue) * 2];
        memcpy(payload, &outside, sizeof(TimeValue));
        memcpy(payload + sizeof(TimeValue), &chamber, sizeof(TimeValue));
        sendMessage(MSG_TIME, payload, sizeof(TimeValue) * 2);
    }
    
    // Send stop signal
    void sendStop() {
        sendMessage(MSG_STOP, nullptr, 0);
    }
    
    // Send acknowledgment
    void sendAck() {
        sendMessage(MSG_ACK, nullptr, 0);
    }
    
    // Receive message (non-blocking)
    bool receiveMessage(uint8_t* msgType, uint8_t* payload, uint16_t* payloadLength) {
        // Check for timeout
        if (receiveIndex > 0 && millis() - lastReceiveTime > 100) {
            receiveIndex = 0;  // Reset on timeout
        }
        
        // Read available bytes
        while (serial->available()) {
            uint8_t byte = serial->read();
            lastReceiveTime = millis();
            
            // Look for start byte
            if (receiveIndex == 0) {
                if (byte == START_BYTE) {
                    receiveBuffer[0] = byte;
                    receiveIndex = 1;
                }
                continue;
            }
            
            // Store byte
            if (receiveIndex < sizeof(receiveBuffer)) {
                receiveBuffer[receiveIndex++] = byte;
                
                // Check if we have minimum message length (Start + Type + Length + Checksum + End)
                if (receiveIndex >= 5) {
                    uint8_t length = receiveBuffer[2];
                    uint16_t expectedLength = 5 + length;  // Full message length
                    
                    if (receiveIndex >= expectedLength) {
                        // Verify end byte
                        if (receiveBuffer[expectedLength - 1] == END_BYTE) {
                            // Calculate checksum
                            uint8_t checksum = 0;
                            for (uint16_t i = 1; i < expectedLength - 2; i++) {
                                checksum ^= receiveBuffer[i];
                            }
                            
                            // Verify checksum
                            if (checksum == receiveBuffer[expectedLength - 2]) {
                                // Valid message received
                                *msgType = receiveBuffer[1];
                                *payloadLength = length;
                                
                                if (length > 0) {
                                    memcpy(payload, &receiveBuffer[3], length);
                                }
                                
                                receiveIndex = 0;  // Reset for next message
                                return true;
                            }
                        }
                        
                        // Invalid message, reset
                        receiveIndex = 0;
                    }
                }
            } else {
                // Buffer overflow, reset
                receiveIndex = 0;
            }
        }
        
        return false;
    }
    
    // Process incoming messages (helper for common patterns)
    bool processIncoming(ExhibitState* state, TimeValue* outside, TimeValue* chamber) {
        uint8_t msgType;
        uint8_t payload[MAX_PAYLOAD_SIZE];
        uint16_t payloadLength;
        
        if (receiveMessage(&msgType, payload, &payloadLength)) {
            switch (msgType) {
                case MSG_STATE:
                    if (payloadLength == sizeof(ExhibitState)) {
                        memcpy(state, payload, sizeof(ExhibitState));
                        return true;
                    }
                    break;
                    
                case MSG_TIME:
                    if (payloadLength == sizeof(TimeValue) * 2) {
                        memcpy(outside, payload, sizeof(TimeValue));
                        memcpy(chamber, payload + sizeof(TimeValue), sizeof(TimeValue));
                        return true;
                    }
                    break;
                    
                case MSG_STOP:
                    // Set flag or call callback
                    return true;
                    
                case MSG_ACK:
                    // Handle acknowledgment if needed
                    return true;
            }
        }
        
        return false;
    }
    
private:
    // Send a message with the protocol format
    void sendMessage(uint8_t msgType, const uint8_t* payload, uint16_t payloadLength) {
        if (payloadLength > MAX_PAYLOAD_SIZE) return;
        
        // Send start byte
        serial->write(START_BYTE);
        
        // Send message type
        serial->write(msgType);
        
        // Send payload length
        serial->write((uint8_t)payloadLength);
        
        // Calculate checksum
        uint8_t checksum = msgType ^ (uint8_t)payloadLength;
        
        // Send payload and update checksum
        for (uint16_t i = 0; i < payloadLength; i++) {
            serial->write(payload[i]);
            checksum ^= payload[i];
        }
        
        // Send checksum
        serial->write(checksum);
        
        // Send end byte
        serial->write(END_BYTE);
        
        // Flush to ensure immediate transmission
        serial->flush();
    }
};

#endif // SERIAL_PROTOCOL_H