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
DisplayManager display; //TODO update per control unit
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