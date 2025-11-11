// ChamberUnit.ino

#include "config.h"

#include "TimeTypes.h"
#include "DisplayManager.h"

// #include <Adafruit_NeoPixel.h>

// #include <SoftwareSerial.h>
#include "SerialProtocol.h"


// Global state
TimeValue normalTime;
TimeValue chamberTime;
ExhibitState state;
DisplayManager display; //TODO update per control unit

// Adafruit_NeoPixel strip(NUM_LEDS, PIN_RGB_LED, NEO_GRB + NEO_KHZ800);

SerialProtocol protocol(&Serial1);

//Button arming
//This can't be based on state, as it is in the control unit, since we must wait for the control unit to set our state
bool stopButtonArmed = false;

void setup() {
  #ifdef ENABLE_SERIAL_LOGGING
    Serial.begin(115200); //USB serial for debugging
  #endif
  #ifdef ENABLE_SERIAL_TO_CONTROL_UNIT
    Serial1.begin(9600); //RX/TX serial to control unit
  #endif
  
  // Initialize inputs
  #ifdef PIN_START_BUTTON
    pinMode(PIN_START_BUTTON,INPUT_PULLUP);
  #endif
  #ifdef PIN_STOP_BUTTON
    pinMode(PIN_STOP_BUTTON,INPUT_PULLUP);
  #endif

  // Test displays
  display.begin();
  display.testPattern();

  // strip.begin();
  // strip.show();
  
  // Signal ready
  state.reset();
}

bool updateReceived = false;

void loop() {
  #ifdef ENABLE_SERIAL_TO_CONTROL_UNIT
    // Check for stop button when applicable
    if(stopButtonArmed) {
      if(digitalRead(PIN_STOP_BUTTON) == LOW) {
        stopButtonArmed = false;
        protocol.sendStop();
      }
    }

    // Receive updates from control unit
    uint8_t msgType, payload[64];
    uint16_t payloadLength;
    if(protocol.receiveMessage(&msgType, payload, &payloadLength)) {
      #ifdef ENABLE_SERIAL_LOGGING
        Serial.print("Received from control: ");
        switch(msgType) {
          case SerialProtocol::MSG_STATE: Serial.print("MSG_STATE"); break;
          case SerialProtocol::MSG_TIME:  Serial.print("MSG_TIME"); break;
          case SerialProtocol::MSG_STOP:  Serial.print("MSG_STOP"); break;
          case SerialProtocol::MSG_ACK:   Serial.print("MSG_ACK"); break;
          case SerialProtocol::MSG_ERROR: Serial.print("MSG_ERROR"); break;
          default: Serial.print("unknown"); break;
        }
        Serial.print(" of length ");
        Serial.println(payloadLength);
      #endif
      switch(msgType) {
        case SerialProtocol::MSG_STATE:
          memcpy(&state, payload, sizeof(ExhibitState));
          updateReceived = true;
          break;
            
        case SerialProtocol::MSG_TIME:
          memcpy(&normalTime, payload, sizeof(TimeValue));
          memcpy(&chamberTime, payload + sizeof(TimeValue), sizeof(TimeValue));
          updateReceived = true;
          break;
      }
    }

    if(updateReceived) {
      updateReceived = false;
      // Update local displays and arm button
      display.updateLEDs(state.current,normalTime);
      display.updateRelays(state.current);
      display.updateMeter(state.chamberRateMs);
      display.updateVibes(state.chamberRateMs);
      // updateRGBEffects();
      display.updateNormalTime(normalTime);
      display.updateChamberTime(chamberTime,state.current);
      if(state.current == ExhibitState::DECELERATION) {
        display.updateSavedTime(normalTime,chamberTime);
        display.updateElapsedTime(state.elapsedMillis);
        stopButtonArmed = true; //make it possible to stop
      }
    }
  #endif
}

void updateRGBEffects() {
    // uint32_t color;
    
    // switch (state.current) {
    //     case ExhibitState::NORMAL:
    //         color = strip.Color(0, 255, 0);  // Green
    //         break;
            
    //     case ExhibitState::DECELERATION: {
    //         // Fade from yellow to red based on rate
    //         uint8_t red = 255;
    //         uint8_t green = map(state.chamberRateQ16, 0, 65536, 0, 255);
    //         color = strip.Color(red, green, 0);
    //         break;
    //     }
        
    //     case ExhibitState::RECOVERY: {
    //         // Fade from red back to green
    //         uint8_t red = map(state.chamberRateQ16, 0, 65536, 255, 0);
    //         uint8_t green = map(state.chamberRateQ16, 0, 65536, 0, 255);
    //         color = strip.Color(red, green, 0);
    //         break;
    //     }
    // }
    
    // // Apply pulsing effect based on time rate
    // uint8_t brightness = 128 + (127 * sin(millis() * state.chamberRateQ16 / 65536000.0));
    // strip.setBrightness(brightness);
    
    // for(int i = 0; i < NUM_LEDS; i++) {
    //     strip.setPixelColor(i, color);
    // }
    // strip.show();
}