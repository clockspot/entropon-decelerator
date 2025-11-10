// SerialTest.ino

#include "config.h"
#include "TimeTypes.h"
#include "DisplayManager.h"

#ifdef RTC_ENABLED
  #include <Wire.h>
  #include <RTClib.h>
#endif

// #include <SoftwareSerial.h>
// #include "SerialProtocol.h"


// Global state
TimeValue normalTime;
TimeValue chamberTime;
ExhibitState state;
DisplayManager display;

#ifdef RTC_ENABLED
  RTC_DS3231 rtc;
#endif

// SerialProtocol protocol(&Serial1);
// #ifdef PIN_CHAMBER_TX
//   SoftwareSerial chamberSerial(PIN_CHAMBER_RX, PIN_CHAMBER_TX);
//   SerialProtocol chamberUnit(&chamberSerial);
//  TODO to support this, SerialProtocol needs to be able to be initiated both with and without a SoftwareSerial specified
// #endif
// #ifdef PIN_PRINTER_TX
//   SoftwareSerial printerSerial(PIN_PRINTER_RX, PIN_PRINTER_TX);
// #endif

void setup() {
  delay(2000);
  #ifdef ENABLE_SERIAL_LOGGING //Serial
    Serial.begin(115200);
    Serial.println("");
    Serial.println("Hello world");
  #endif
  #ifdef ENABLE_SERIAL_TO_CHAMBER_UNIT //Serial1
    Serial1.begin(9600);
  #endif
  #ifdef PIN_BUTTON
    pinMode(PIN_BUTTON,INPUT_PULLUP);
  #endif
  // Test displays
  display.begin();
  display.testPattern();
}

uint32_t buttonLastPress=0;
uint32_t buttonPresses=0;
void loop() {

  //If the button has been pressed, increment buttonPresses, set display and send to other unit
  if (digitalRead(PIN_BUTTON) == LOW && (millis()-buttonLastPress > 1000)) {
    buttonLastPress = millis();
    buttonPresses++;
    if(buttonPresses>9) buttonPresses = 0;
    
    display.displayDiagnostics(buttonPresses);
    #ifdef ENABLE_SERIAL_LOGGING //Serial
      Serial.write(buttonPresses + '0'); //To serial monitor
    #endif
    #ifdef ENABLE_SERIAL_TO_CHAMBER_UNIT //Serial1
      Serial1.write(buttonPresses + '0'); //send as ASCII
    #endif
  }

  //If we have input from computer, set display and send to other unit
  #ifdef ENABLE_SERIAL_LOGGING //Serial
    if(Serial.available()) {
      char incomingChar = Serial.read();
      if(incomingChar >= '0' && incomingChar <= '9') {
        int digit = incomingChar - '0';
        display.displayDiagnostics(digit);
        Serial.print("Received from USB: ");
        Serial.println(digit);
        #ifdef ENABLE_SERIAL_TO_CHAMBER_UNIT //Serial1
          Serial1.write(digit + '0'); //send as ASCII
        #endif
      }
    }
  #endif

  //If we have input from other unit, set display and send to computer
  #ifdef ENABLE_SERIAL_TO_CHAMBER_UNIT //Serial1
    if(Serial1.available()) {
      char incomingChar = Serial1.read();
      if(incomingChar >= '0' && incomingChar <= '9') {
        int digit = incomingChar - '0';
        display.displayDiagnostics(digit);
        Serial.print("Received from RX: ");
        Serial.println(digit);
      }
    }
  #endif

  delay(200);
}