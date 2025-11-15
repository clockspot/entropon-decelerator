// RTCTest.ino

#include "config.h"

#ifdef ENABLE_RTC
  #include <Wire.h>
  #include <RTClib.h>
  RTC_DS3231 rtc;
#endif

void setup() {
  #ifdef ENABLE_SERIAL_LOGGING //Serial
    Serial.begin(115200);
    delay(1000);
    Serial.println("");
    Serial.println("Hello world. Enter 'r' to set RTC.");
  #endif
  delay(5000);
  #ifdef ENABLE_RTC
    if(rtc.begin()) {
      setRTC(); //See if we have input to set RTC by
      // syncTimeFromRTC();
    }
  #endif
}

void loop() {
  Serial.print("RTC time: ");
  DateTime tod = rtc.now();
  if(tod.hour()<10) Serial.print("0");
  Serial.print(tod.hour());
  Serial.print(":");
  if(tod.minute()<10) Serial.print("0");
  Serial.print(tod.minute());
  Serial.print(":");
  if(tod.second()<10) Serial.print("0");
  Serial.print(tod.second());
  Serial.println();
  delay(1000);
}

void setRTC() {
  #ifdef ENABLE_RTC
    #ifdef ENABLE_SERIAL_LOGGING
      //Check for serial console input to set RTC
      if(Serial.available()) {
        char incomingChar = Serial.read();
        while(Serial.available()) Serial.read(); //dump the rest
        if(incomingChar != 'r') {
          Serial.print("Unknown command: ");
          Serial.println(incomingChar);
          return;
        }
      } else {
        Serial.println("No command received.");
        return;
      }
      //setting!
      int hr = 0;
      Serial.println("Setting RTC. Enter hour:");
      while(1) {
        if(Serial.available()) {
          hr = Serial.parseInt()%24;
          while(Serial.available()) Serial.read();
          break;
        }
      }
      int min = 0;
      Serial.println("Enter minute, at top of minute:");
      while(1) {
        if(Serial.available()) {
          min = Serial.parseInt()%60;
          while(Serial.available()) Serial.read();
          break;
        }
      }
      rtc.adjust(DateTime(2025, 1, 1, hr, min, 0));
    #endif
  #endif
}