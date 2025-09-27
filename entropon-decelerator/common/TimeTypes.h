// // TimeTypes.h - Common data structures
// struct TimeValue {
//     uint32_t millisSinceMidnight;
    
//     void addMillis(uint32_t delta);
//     void normalize();  // Handle midnight rollover
//     uint8_t getHours() const;
//     uint8_t getMinutes() const;
//     uint8_t getSeconds() const;
//     uint16_t getMillis() const;
// };

// struct ExhibitState {
//     enum State { NORMAL, DECELERATION, RECOVERY };
//     State current;
//     uint32_t elapsedMillis;
//     int32_t chamberRateQ16;  // Fixed-point Q16 format
// };

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