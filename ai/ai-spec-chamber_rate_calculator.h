// ==========================================
// Optimized updateChamberRate() function
// Uses integer math only - no floating point!
// ==========================================

// Constants for rate calculation
#define RATE_NORMAL 1000        // 1000 ms per outside second = normal
#define RATE_MIN 100           // 100 ms per outside second = 90% slower
#define RATE_MAX 1000          // Maximum rate (normal speed)

// Lookup table for exponential curves (precomputed)
// These approximate exp(-t) for t from 0 to 5 in steps of 0.1
// Scaled to 0-1000 for integer math
const uint16_t EXP_LOOKUP[] PROGMEM = {
    1000, 905, 819, 741, 670, 607, 549, 497, 449, 406,  // 0.0 - 0.9
    368, 333, 301, 273, 247, 223, 202, 183, 165, 150,   // 1.0 - 1.9
    135, 122, 111, 100, 91, 82, 74, 67, 61, 55,         // 2.0 - 2.9
    50, 45, 41, 37, 33, 30, 27, 25, 22, 20,            // 3.0 - 3.9
    18, 17, 15, 14, 12, 11, 10, 9, 8, 7                // 4.0 - 4.9
};

// Fast integer-only version of updateChamberRate
void updateChamberRate() {
    // Read potentiometers (0-1023 range)
    uint16_t potMaxNeg = analogRead(PIN_POT_MAX_NEG);     // Controls deceleration speed
    uint16_t potMaxPos = analogRead(PIN_POT_MAX_POS);     // Controls recovery speed  
    uint16_t potMinRate = analogRead(PIN_POT_MIN_RATE);   // Controls minimum chamber rate
    
    // Convert potMinRate to minimum milliseconds per second (100-1000 range)
    // When pot is 0: minRate = 100 (very slow)
    // When pot is 1023: minRate = 1000 (normal speed)
    uint16_t minRate = 100 + ((uint32_t)potMinRate * 900) / 1023;
    
    switch (state.current) {
        case ExhibitState::NORMAL:
            state.chamberRateMs = RATE_NORMAL;  // 1000 ms per outside second
            break;
            
        case ExhibitState::DECELERATION: {
            // Time constant: faster when pot is high (0.5 to 5.0 seconds)
            // Higher potMaxNeg = faster deceleration
            uint32_t timeConstant = 500 + ((uint32_t)(1023 - potMaxNeg) * 4500) / 1023;
            
            // Calculate table index based on elapsed time and time constant
            uint32_t tableIndex = (state.elapsedMillis * 50) / timeConstant;
            if (tableIndex > 49) tableIndex = 49;  // Clamp to table size
            
            // Look up exponential value (0-1000)
            uint16_t expValue = pgm_read_word(&EXP_LOOKUP[tableIndex]);
            
            // Calculate rate: rate = minRate + (1000 - minRate) * exp(-t)
            state.chamberRateMs = minRate + ((uint32_t)(RATE_NORMAL - minRate) * expValue) / 1000;
            break;
        }
        
        case ExhibitState::RECOVERY: {
            // Time constant for recovery
            uint32_t timeConstant = 500 + ((uint32_t)(1023 - potMaxPos) * 4500) / 1023;
            
            // Calculate table index
            uint32_t tableIndex = (state.elapsedMillis * 50) / timeConstant;
            if (tableIndex > 49) tableIndex = 49;
            
            // Look up exponential value
            uint16_t expValue = pgm_read_word(&EXP_LOOKUP[tableIndex]);
            
            // For recovery, we need 1 - exp(-t)
            uint16_t oneMinusExp = 1000 - expValue;
            
            // Calculate rate: rate = startRate + (1000 - startRate) * (1 - exp(-t))
            // Note: startRate should be saved when entering recovery
            state.chamberRateMs = state.savedMinRate + 
                ((uint32_t)(RATE_NORMAL - state.savedMinRate) * oneMinusExp) / 1000;
            break;
        }
    }
}

// ==========================================
// Updated ExhibitState structure
// ==========================================

struct ExhibitState {
    enum State { 
        NORMAL = 0, 
        DECELERATION = 1, 
        RECOVERY = 2 
    };
    
    State current;
    uint32_t elapsedMillis;
    uint16_t chamberRateMs;      // Milliseconds per outside second (1000 = normal)
    uint16_t savedMinRate;        // Rate when deceleration ended (for recovery)
    
    // Initialize to default state
    void reset() {
        current = NORMAL;
        elapsedMillis = 0;
        chamberRateMs = RATE_NORMAL;
        savedMinRate = RATE_NORMAL;
    }
    
    // Get rate as a fraction (for display purposes)
    float getRateAsFloat() const {
        return (float)chamberRateMs / 1000.0f;
    }
    
    // Get seconds lost per second (for meter display)
    float getSecondsLostPerSecond() const {
        return 1.0f - getRateAsFloat();
    }
};

// ==========================================
// Updated time calculation in loop()
// ==========================================

void updateChamberTime(uint32_t deltaMillis) {
    // Calculate chamber time delta using the rate
    // chamberDelta = deltaMillis * (chamberRateMs / 1000)
    // Using integer math: chamberDelta = (deltaMillis * chamberRateMs) / 1000
    
    uint32_t chamberDelta = ((uint32_t)deltaMillis * state.chamberRateMs) / 1000;
    chamberTime.addMillis(chamberDelta);
}

// ==========================================
// Alternative: Even simpler linear version
// ==========================================

void updateChamberRateLinear() {
    // Read potentiometers
    uint16_t potMaxNeg = analogRead(PIN_POT_MAX_NEG);
    uint16_t potMaxPos = analogRead(PIN_POT_MAX_POS);  
    uint16_t potMinRate = analogRead(PIN_POT_MIN_RATE);
    
    // Minimum rate from pot (100-1000 ms per second)
    uint16_t minRate = 100 + ((uint32_t)potMinRate * 900) / 1023;
    
    // Deceleration/recovery duration from pots (2-20 seconds)
    uint32_t decelDuration = 2000 + ((uint32_t)(1023 - potMaxNeg) * 18000) / 1023;
    uint32_t recoveryDuration = 2000 + ((uint32_t)(1023 - potMaxPos) * 18000) / 1023;
    
    switch (state.current) {
        case ExhibitState::NORMAL:
            state.chamberRateMs = RATE_NORMAL;
            break;
            
        case ExhibitState::DECELERATION: {
            if (state.elapsedMillis >= decelDuration) {
                // Reached minimum rate
                state.chamberRateMs = minRate;
            } else {
                // Linear interpolation from 1000 to minRate
                uint32_t progress = (state.elapsedMillis * 1000) / decelDuration;
                state.chamberRateMs = RATE_NORMAL - ((RATE_NORMAL - minRate) * progress) / 1000;
            }
            break;
        }
        
        case ExhibitState::RECOVERY: {
            if (state.elapsedMillis >= recoveryDuration) {
                // Back to normal
                state.chamberRateMs = RATE_NORMAL;
            } else {
                // Linear interpolation from savedMinRate to 1000
                uint32_t progress = (state.elapsedMillis * 1000) / recoveryDuration;
                state.chamberRateMs = state.savedMinRate + 
                    ((RATE_NORMAL - state.savedMinRate) * progress) / 1000;
            }
            break;
        }
    }
}

// ==========================================
// State transition handler update
// ==========================================

void handleStateTransitions() {
    static uint32_t decelStartTime = 0;
    static uint32_t recoverStartTime = 0;
    
    switch (state.current) {
        case ExhibitState::NORMAL:
            if (digitalRead(PIN_START_BUTTON) == LOW) {
                state.current = ExhibitState::DECELERATION;
                state.elapsedMillis = 0;
                decelStartTime = millis();
            }
            break;
            
        case ExhibitState::DECELERATION:
            state.elapsedMillis = millis() - decelStartTime;
            
            // Check for stop button or minimum rate reached
            if (digitalRead(PIN_STOP_BUTTON) == LOW || 
                state.chamberRateMs <= 110) {  // Within 10% of minimum
                
                // Save the current rate for recovery calculation
                state.savedMinRate = state.chamberRateMs;
                
                state.current = ExhibitState::RECOVERY;
                state.elapsedMillis = 0;
                recoverStartTime = millis();
                
                // Freeze displays
                display.setDisplayFreeze(true);
                
                printCertificate();
            }
            break;
            
        case ExhibitState::RECOVERY:
            state.elapsedMillis = millis() - recoverStartTime;
            
            // Check if back to normal (within 1% of normal rate)
            if (state.chamberRateMs >= 990) {
                state.current = ExhibitState::NORMAL;
                state.elapsedMillis = 0;
                
                // Unfreeze displays
                display.setDisplayFreeze(false);
                
                // Resync times
                chamberTime = outsideTime;
            }
            break;
    }
}

// ==========================================
// Updated DisplayManager meter function
// ==========================================

void updateMeterForMillisRate(uint16_t rateMs) {
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