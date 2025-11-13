// ==========================================
// RTCMillis.h - RTC-synchronized millisecond timing
// ==========================================

#ifndef RTC_MILLIS_H
#define RTC_MILLIS_H

#include <Arduino.h>
#include <RTClib.h>

class RTCMillis {
private:
    RTC_DS3231* rtc;
    
    // Synchronization points
    uint32_t lastSyncMillis;      // millis() value at last RTC sync
    uint32_t lastSyncRTCSeconds;  // RTC seconds at last sync
    DateTime lastSyncDateTime;     // Full DateTime at last sync
    
    // Drift correction
    float millisPerMillis;         // Correction factor (normally 1.0)
    uint32_t lastDriftCheckMillis;
    uint32_t lastDriftCheckRTCSeconds;
    
    // Configuration
    static const uint32_t SYNC_INTERVAL = 10000;      // Resync every ~60~ 10 seconds
    static const uint32_t DRIFT_CHECK_INTERVAL = 30000; // Check drift every ~5 minutes~ 30 seconds
    static const uint32_t MIN_DRIFT_SAMPLES = 10;     // Minimum time between drift calculations
    
public:
    RTCMillis(RTC_DS3231* rtcRef) : rtc(rtcRef), millisPerMillis(1.0) {
        sync();
        lastDriftCheckMillis = lastSyncMillis;
        lastDriftCheckRTCSeconds = lastSyncRTCSeconds;
    }
    
    // Force synchronization with RTC
    void sync() {
        DateTime now = rtc->now();
        lastSyncDateTime = now;
        lastSyncRTCSeconds = now.hour() * 3600UL + now.minute() * 60UL + now.second();
        lastSyncMillis = millis();
    }
    
    // Get corrected milliseconds since midnight
    uint32_t getMillisSinceMidnight() {
        uint32_t currentMillis = millis();
        uint32_t millisElapsed = currentMillis - lastSyncMillis;
        
        // Apply drift correction
        uint32_t correctedMillisElapsed = (uint32_t)(millisElapsed * millisPerMillis);
        
        // Check if we should resync with RTC
        if (millisElapsed > SYNC_INTERVAL) {
            updateDriftCorrection();
            sync();
            return getMillisSinceMidnight(); // Recursive call with fresh sync
        }
        
        // Calculate current time
        uint32_t currentSeconds = lastSyncRTCSeconds + (correctedMillisElapsed / 1000);
        uint32_t currentMillisInSecond = correctedMillisElapsed % 1000;
        
        // Handle day rollover
        if (currentSeconds >= 86400) {
            currentSeconds %= 86400;
        }
        
        return currentSeconds * 1000UL + currentMillisInSecond;
    }
    
    // Get corrected "millis" value (like Arduino millis() but RTC-corrected)
    uint32_t getCorrectedMillis() {
        uint32_t currentMillis = millis();
        uint32_t millisElapsed = currentMillis - lastSyncMillis;
        
        // Apply drift correction
        return lastSyncMillis + (uint32_t)(millisElapsed * millisPerMillis);
    }
    
    // Update drift correction factor
    void updateDriftCorrection() {
        uint32_t currentMillis = millis();
        uint32_t millisElapsed = currentMillis - lastDriftCheckMillis;
        
        // Only update drift if enough time has passed
        if (millisElapsed < DRIFT_CHECK_INTERVAL) {
            return;
        }
        
        // Get actual RTC time elapsed
        DateTime now = rtc->now();
        uint32_t currentRTCSeconds = now.hour() * 3600UL + now.minute() * 60UL + now.second();
        
        // Handle midnight rollover
        int32_t rtcSecondsElapsed = currentRTCSeconds - lastDriftCheckRTCSeconds;
        if (rtcSecondsElapsed < 0) {
            rtcSecondsElapsed += 86400; // Add a day
        }
        
        // Calculate drift correction
        // If RTC says 60 seconds passed but millis says 61000ms, 
        // then millisPerMillis = 60000/61000 = 0.9836
        if (millisElapsed > 0 && rtcSecondsElapsed >= MIN_DRIFT_SAMPLES) {
            float actualMillis = rtcSecondsElapsed * 1000.0;
            float reportedMillis = (float)millisElapsed;
            
            // Apply exponential smoothing to avoid sudden jumps
            float newCorrection = actualMillis / reportedMillis;
            millisPerMillis = (millisPerMillis * 0.7) + (newCorrection * 0.3);
            
            // Clamp to reasonable range (±5% drift)
            if (millisPerMillis > 1.05) millisPerMillis = 1.05;
            if (millisPerMillis < 0.95) millisPerMillis = 0.95;
        }
        
        // Update drift check reference point
        lastDriftCheckMillis = currentMillis;
        lastDriftCheckRTCSeconds = currentRTCSeconds;
    }
    
    // Get current drift correction factor (for diagnostics)
    float getDriftCorrection() {
        return millisPerMillis;
    }
    
    // Get time since last sync (for diagnostics)
    uint32_t getTimeSinceSync() {
        return millis() - lastSyncMillis;
    }
};

#endif // RTC_MILLIS_H