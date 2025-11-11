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
    uint32_t getHHMMSS() const {
        return  (millisSinceMidnight / MILLIS_PER_HOUR) % 24 * 10000L + 
                (millisSinceMidnight / MILLIS_PER_MINUTE) % 60 * 100L +
                (millisSinceMidnight / MILLIS_PER_SECOND) % 60;
    }

    bool getBlink() const {
        return (millisSinceMidnight % MILLIS_PER_SECOND / 500)>=1;
    }

    bool getLongBlink() const {
        return (millisSinceMidnight % MILLIS_PER_SECOND / 667)>=1;
    }
    
    uint32_t getTotalSeconds() const {
        return millisSinceMidnight / MILLIS_PER_SECOND;
    }
    
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
    uint16_t chamberRateMs;      // Milliseconds per normal second (1000 = normal)
    uint16_t savedMinRate;        // Rate when deceleration ended (for recovery)
    
    // Initialize to default state
    void reset() {
        current = NORMAL;
        elapsedMillis = 0;
        chamberRateMs = 1000;
        savedMinRate = 1000;
    }
};

#endif // TIME_TYPES_H