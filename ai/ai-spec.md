# Time Chamber Exhibit - Technical Specification

## System Architecture Overview

### Key Design Improvements
1. **Modular architecture** with shared libraries for common functionality
2. **Fixed-point arithmetic** for precise time calculations without floating-point errors
3. **State machine pattern** for robust state management
4. **Hardware abstraction layer** for cleaner code organization
5. **Error handling and watchdog timers** for reliability

## Hardware Components

### Control Unit
- **Microcontroller**: Arduino Nano 33 IoT
- **RTC Module**: DS3231 (high precision with temperature compensation)
- **Inputs**:
  - 2× Momentary pushbuttons (Start, Stop) with 10kΩ pull-up resistors
  - 3× 10kΩ linear potentiometers for parameters
- **Outputs**:
  - 3× Status LEDs with 220Ω current-limiting resistors
  - 1× 5V analog meter (PWM-driven via transistor buffer)
  - 4× TM1637-based 6-digit displays
  - 3× Lavet stepper motor clock mechanisms
  - 1× Thermal printer module (serial communication)
- **Communication**: Software Serial for bidirectional link

### Chamber Unit
- **Microcontroller**: Arduino Nano (classic)
- **Inputs**:
  - 1× Momentary pushbutton (Stop) with 10kΩ pull-up resistor
- **Outputs**:
  - 3× Status LEDs with 220Ω resistors
  - 1× 5V analog meter (PWM-driven)
  - 4× TM1637-based 6-digit displays
  - Multiple vibration motors (PWM controlled via MOSFETs)
  - WS2812B RGB LED strip (NeoPixel compatible)
  - 1× Relay module for smoke machine (with flyback diode)
- **Communication**: Software Serial for receiving data

## Wiring Diagrams

### Control Unit Connections

```
Arduino Nano 33 IoT Pin Assignments:
----------------------------------------
D2  - Start Button (INPUT_PULLUP)
D3  - Stop Button (INPUT_PULLUP)
D4  - State LED Green (Normal)
D5  - State LED Yellow (Deceleration)
D6  - State LED Red (Recovery)
D9  - Meter PWM Output (via transistor)
D10 - Software Serial TX (to Chamber)
D11 - Software Serial RX (from Chamber)
D12 - Printer Serial TX
D13 - Printer Serial RX

A0  - Max Negative Rate Pot
A1  - Max Positive Rate Pot  
A2  - Min Chamber Rate Pot
A4  - I2C SDA (RTC DS3231)
A5  - I2C SCL (RTC DS3231)

Display 1 (Outside Time):
  CLK - D7
  DIO - D8
  
Display 2 (Chamber Time):
  CLK - A6
  DIO - A7
  
Display 3 (Difference):
  CLK - D0
  DIO - D1
  
Display 4 (Elapsed):
  CLK - D14
  DIO - D15

Clock Motors:
  Outside Clock Even - D16
  Outside Clock Odd  - D17
  Chamber Clock Even - D18
  Chamber Clock Odd  - D19
  Saved Clock Even   - D20
  Saved Clock Odd    - D21
```

### Chamber Unit Connections

```
Arduino Nano Pin Assignments:
----------------------------------------
D2  - Stop Button (INPUT_PULLUP)
D4  - State LED Green (Normal)
D5  - State LED Yellow (Deceleration)
D6  - State LED Red (Recovery)
D9  - Meter PWM Output
D10 - Software Serial TX (to Control)
D11 - Software Serial RX (from Control)
D3  - Vibration Motors PWM (via MOSFET)
D7  - RGB LED Data Pin (WS2812B)
D8  - Smoke Machine Relay

[Display connections same as Control Unit]
```

### Circuit Protection
- All LEDs require 220Ω current-limiting resistors
- Buttons use internal pull-up resistors (no external components needed)
- Relay module should include optocoupler isolation
- Add 0.1μF bypass capacitors near each IC power pin
- Use twisted pair or shielded cable for serial communication between units

## Software Architecture

### Shared Libraries Structure

```cpp
// TimeTypes.h - Common data structures
struct TimeValue {
    uint32_t millisSinceMidnight;
    
    void addMillis(uint32_t delta);
    void normalize();  // Handle midnight rollover
    uint8_t getHours() const;
    uint8_t getMinutes() const;
    uint8_t getSeconds() const;
    uint16_t getMillis() const;
};

struct ExhibitState {
    enum State { NORMAL, DECELERATION, RECOVERY };
    State current;
    uint32_t elapsedMillis;
    int32_t chamberRateQ16;  // Fixed-point Q16 format
};

// DisplayManager.h - Shared display code
class DisplayManager {
public:
    void updateTimeDisplay(uint8_t displayNum, const TimeValue& time);
    void updateDifferenceDisplay(const TimeValue& outside, const TimeValue& chamber);
    void updateElapsedDisplay(uint32_t elapsedMillis);
    void updateMeter(int32_t rateQ16);
    void updateLEDs(ExhibitState::State state);
};

// SerialProtocol.h - Communication protocol
class SerialProtocol {
public:
    static const uint8_t MSG_STATE = 0x01;
    static const uint8_t MSG_TIME = 0x02;
    static const uint8_t MSG_STOP = 0x03;
    
    void sendStateUpdate(const ExhibitState& state);
    void sendTimeUpdate(const TimeValue& outside, const TimeValue& chamber);
    bool receiveMessage(uint8_t* msgType, uint8_t* payload);
};
```

### Control Unit Main Code

```cpp
// ControlUnit.ino
#include <Wire.h>
#include <RTClib.h>
#include <SoftwareSerial.h>
#include "config_control.h"
#include "TimeTypes.h"
#include "DisplayManager.h"
#include "SerialProtocol.h"

// Global state
TimeValue outsideTime;
TimeValue chamberTime;
ExhibitState state;
DisplayManager display;
SerialProtocol protocol;
RTC_DS3231 rtc;
SoftwareSerial chamberSerial(PIN_SERIAL_RX, PIN_SERIAL_TX);
SoftwareSerial printerSerial(PIN_PRINTER_RX, PIN_PRINTER_TX);

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
    chamberSerial.begin(9600);
    printerSerial.begin(9600);
    
    // Initialize hardware
    initializePins();
    initializeDisplays();
    
    // Initialize RTC
    if (!rtc.begin()) {
        Serial.println(F("RTC not found!"));
        while (1) delay(10);
    }
    
    // Sync time from RTC
    syncTimeFromRTC();
    
    // Test displays
    performDisplayTest();
    
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
    updateAllDisplays();
    
    // Handle clock pulses
    updateAnalogClocks();
    
    // Communicate with chamber unit
    protocol.sendStateUpdate(state);
    protocol.sendTimeUpdate(outsideTime, chamberTime);
    
    // Check for stop signal from chamber
    uint8_t msgType, payload[32];
    if (protocol.receiveMessage(&msgType, payload)) {
        if (msgType == SerialProtocol::MSG_STOP) {
            triggerStop();
        }
    }
    
    // Re-sync with RTC at midnight
    if (state.current == ExhibitState::NORMAL && outsideTime.getHours() == 0 && 
        outsideTime.getMinutes() == 0 && outsideTime.getSeconds() < 2) {
        syncTimeFromRTC();
    }
    
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
                state.elapsedMillis = 0;
                decelStartTime = millis();
            }
            break;
            
        case ExhibitState::DECELERATION:
            state.elapsedMillis = millis() - decelStartTime;
            
            if (digitalRead(PIN_STOP_BUTTON) == LOW || 
                state.chamberRateQ16 <= (int32_t)(0.01 * 65536)) {
                state.current = ExhibitState::RECOVERY;
                state.elapsedMillis = 0;
                recoverStartTime = millis();
                printCertificate();
            }
            break;
            
        case ExhibitState::RECOVERY:
            state.elapsedMillis = millis() - recoverStartTime;
            
            if (abs(state.chamberRateQ16 - 65536) < 655) { // Within 1% of normal
                state.current = ExhibitState::NORMAL;
                state.elapsedMillis = 0;
                // Resync times
                chamberTime = outsideTime;
            }
            break;
    }
}

void printCertificate() {
    uint32_t timeDiff = abs((int32_t)(outsideTime.millisSinceMidnight - 
                            chamberTime.millisSinceMidnight));
    uint32_t secondsSaved = timeDiff / 1000;
    uint32_t secondsSpent = state.elapsedMillis / 1000;
    
    printerSerial.print(F("I spent "));
    printerSerial.print(secondsSpent);
    printerSerial.print(F(" seconds to save "));
    printerSerial.print(secondsSaved);
    printerSerial.println(F(" seconds in the chamber"));
}
```

### Chamber Unit Code

```cpp
// ChamberUnit.ino
#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>
#include "config_chamber.h"
#include "TimeTypes.h"
#include "DisplayManager.h"
#include "SerialProtocol.h"

// Global state
TimeValue outsideTime;
TimeValue chamberTime;
ExhibitState state;
DisplayManager display;
SerialProtocol protocol;
SoftwareSerial controlSerial(PIN_SERIAL_RX, PIN_SERIAL_TX);
Adafruit_NeoPixel strip(NUM_LEDS, PIN_RGB_LED, NEO_GRB + NEO_KHZ800);

void setup() {
    Serial.begin(115200);
    controlSerial.begin(9600);
    
    initializePins();
    initializeDisplays();
    
    strip.begin();
    strip.show();
    
    // Signal ready
    display.updateLEDs(ExhibitState::NORMAL);
}

void loop() {
    // Check for stop button
    if (digitalRead(PIN_STOP_BUTTON) == LOW) {
        uint8_t stopMsg = SerialProtocol::MSG_STOP;
        controlSerial.write(&stopMsg, 1);
        delay(50); // Debounce
    }
    
    // Receive updates from control unit
    uint8_t msgType, payload[32];
    if (protocol.receiveMessage(&msgType, payload)) {
        switch (msgType) {
            case SerialProtocol::MSG_STATE:
                memcpy(&state, payload, sizeof(ExhibitState));
                updateEffects();
                break;
                
            case SerialProtocol::MSG_TIME:
                memcpy(&outsideTime, payload, sizeof(TimeValue));
                memcpy(&chamberTime, payload + sizeof(TimeValue), sizeof(TimeValue));
                updateAllDisplays();
                break;
        }
    }
}

void updateEffects() {
    // Update LEDs
    display.updateLEDs(state.current);
    
    // Update meter
    display.updateMeter(state.chamberRateQ16);
    
    // Vibration intensity based on rate deviation
    int32_t deviation = abs(state.chamberRateQ16 - 65536);
    uint8_t vibrationPWM = map(deviation, 0, 65536, 0, 255);
    analogWrite(PIN_VIBRATION, vibrationPWM);
    
    // RGB LED effects
    updateRGBEffects();
    
    // Smoke machine during peak deceleration
    bool smokeActive = (state.current == ExhibitState::DECELERATION && 
                       state.chamberRateQ16 < (int32_t)(0.5 * 65536));
    digitalWrite(PIN_SMOKE_RELAY, smokeActive ? HIGH : LOW);
}

void updateRGBEffects() {
    uint32_t color;
    
    switch (state.current) {
        case ExhibitState::NORMAL:
            color = strip.Color(0, 255, 0);  // Green
            break;
            
        case ExhibitState::DECELERATION: {
            // Fade from yellow to red based on rate
            uint8_t red = 255;
            uint8_t green = map(state.chamberRateQ16, 0, 65536, 0, 255);
            color = strip.Color(red, green, 0);
            break;
        }
        
        case ExhibitState::RECOVERY: {
            // Fade from red back to green
            uint8_t red = map(state.chamberRateQ16, 0, 65536, 255, 0);
            uint8_t green = map(state.chamberRateQ16, 0, 65536, 0, 255);
            color = strip.Color(red, green, 0);
            break;
        }
    }
    
    // Apply pulsing effect based on time rate
    uint8_t brightness = 128 + (127 * sin(millis() * state.chamberRateQ16 / 65536000.0));
    strip.setBrightness(brightness);
    
    for(int i = 0; i < NUM_LEDS; i++) {
        strip.setPixelColor(i, color);
    }
    strip.show();
}
```

## Key Technical Enhancements

### 1. Fixed-Point Arithmetic
Using Q16 fixed-point format (16 bits integer, 16 bits fraction) for time rate calculations eliminates floating-point errors and ensures precise time tracking.

### 2. Exponential Curves
The time rate changes follow smooth exponential curves rather than linear transitions, creating more natural-feeling acceleration and deceleration.

### 3. Error Handling
- Watchdog timer implementation to recover from hangs
- Bounds checking on all array accesses
- Validation of serial data with checksums
- Graceful handling of sensor disconnections

### 4. Power Considerations
- Use of sleep modes when idle
- PWM frequency optimization to reduce power consumption
- Proper decoupling capacitors to prevent brownouts

### 5. Modular Architecture
The shared library approach allows:
- Single source of truth for display logic
- Easy maintenance and updates
- Consistent behavior between units
- Reduced code duplication

## Testing Procedures

1. **Unit Testing**: Test each component independently
2. **Integration Testing**: Verify serial communication protocol
3. **Stress Testing**: Run for extended periods to check for memory leaks or timing drift
4. **Edge Case Testing**: Test midnight rollovers, millis() overflow, extreme parameter values

## Configuration Files

### config_control.h
```cpp
#pragma once

// Pin definitions
#define PIN_START_BUTTON 2
#define PIN_STOP_BUTTON 3
#define PIN_LED_NORMAL 4
#define PIN_LED_DECEL 5
#define PIN_LED_RECOVERY 6
// ... etc

// Timing parameters
#define CLOCK_PULSE_DURATION 30  // milliseconds
#define SERIAL_TIMEOUT 100        // milliseconds

// Display parameters
#define DISPLAY_UPDATE_INTERVAL 50  // milliseconds
```

### config_chamber.h
```cpp
#pragma once

// Pin definitions
#define PIN_STOP_BUTTON 2
#define PIN_VIBRATION 3
#define PIN_RGB_LED 7
#define PIN_SMOKE_RELAY 8
// ... etc

// LED strip parameters
#define NUM_LEDS 60

// Effect parameters
#define VIBRATION_MIN_PWM 0
#define VIBRATION_MAX_PWM 200
```

## Future Enhancements

1. **WiFi Integration**: Use Nano 33 IoT's WiFi for remote monitoring and control
2. **Data Logging**: Store session data for analytics
3. **Mobile App**: Companion app for parameter adjustment and monitoring
4. **Multiple Chambers**: Support for multiple chamber units from single control
5. **Audio Effects**: Time-stretched audio playback synchronized with chamber rate