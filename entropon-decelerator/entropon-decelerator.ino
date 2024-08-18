// https://github.com/clockspot/entropon-decelerator
// Sketch by Luke McKenzie (luke@theclockspot.com)

#include <arduino.h>
#include "entropon-decelerator.h"

////////// Includes //////////

// These modules are used per the available hardware and features enabled in the config file.
// The disp and rtc options are mutually exclusive and define the same functions.
#ifdef DISPLAY_HT16K33
  #include "dispHT16K33.h" //if DISPLAY_HT16K33 is defined in config - for an I2C 7-segment LED display
#endif
#ifdef RTC_IS_DS3231
  #include "rtcDS3231.h" //if RTC_IS_DS3231 is defined in config – for an I2C DS3231 RTC module
#endif
#ifdef RTC_IS_MILLIS_NO
  #include "rtcMillis.h" //if RTC_IS_MILLIS is defined in config – for a fake RTC based on millis
#endif

#ifdef NETWORK_SSID
  #include "networkNINA.h" //enables WiFi/web-based config/NTP sync on Nano 33 IoT WiFiNINA
#endif

#define SHOW_SERIAL


////////// Main code control //////////

void setup(){
  delay(5000); //for development, just in case it boot loops
  #ifdef SHOW_SERIAL
    Serial.begin(115200);
    #ifdef SAMD_SERIES
      while(!Serial);
    #else
      delay(1);
    #endif
    Serial.println(F("Hello world"));
  #endif
  // rtcInit();
  initDisplay();
  startTime();
  // initOutputs(); //depends on some EEPROM settings
  // initInputs();
  // initNetwork();

} //end setup()

void loop(){
  // //checkRTC(false); //if clock has ticked, decrement timer if running, and updateDisplay
  // millisApplyDrift();
  // checkInputs(); //if inputs have changed, this will do things + updateDisplay as needed
  // #ifdef NETWORK_H
  //   cycleNetwork();
  // #endif
  // cycleTimer();
  // cycleDisplay();
  // cycleSignal();
  updateTime();
}



////////// Input handling and value setting //////////

//ctrlEvt() has moved to input*.cpp, since how it behaves is a function of the input controls available


////////// Timing and timed events //////////

//unsigned long millisLast = 0;
unsigned long millisStart = 0;
unsigned long millisOuterTimeLast = 0;
unsigned long millisInnerTimeLast = 0;
int millisOuterTimeTick = 1000;
int millisInnerTimeTick = 1100;

void startTime() {
  millisStart = 0; //practice setting this back a bit
}

void updateTime() {
  unsigned long now = millis()-millisStart;
  if(now-millisOuterTimeLast>millisOuterTimeTick) {
    //todo modify this to catch colon changes
    millisOuterTimeLast+=millisOuterTimeTick;
    byte h = (millisOuterTimeLast%86400000)/3600000;
    byte m = (millisOuterTimeLast%3600000)/60000;
    byte s = (millisOuterTimeLast%60000)/1000;
    #ifdef SHOW_SERIAL
      Serial.print(F("Outer tick at "));
      Serial.print(h,DEC);
      Serial.print(F(":"));
      if(m<10) Serial.print(F("0"));
      Serial.print(m,DEC);
      Serial.print(F(":"));
      if(s<10) Serial.print(F("0"));
      Serial.print(s,DEC);
      Serial.println();
    #endif
    editDisplay(0,h,m,s);
  }
  if(now-millisInnerTimeLast>millisInnerTimeTick) {
    //todo modify this to catch colon changes
    millisInnerTimeLast+=millisInnerTimeTick;
    unsigned long innerTime = millisInnerTimeLast/millisInnerTimeTick;
    byte h = (innerTime%86400)/3600;
    byte m = (innerTime%3600)/60;
    byte s = (innerTime%60)/1;
    #ifdef SHOW_SERIAL
      Serial.print(F("Inner tick at "));
      Serial.print(h,DEC);
      Serial.print(F(":"));
      if(m<10) Serial.print(F("0"));
      Serial.print(m,DEC);
      Serial.print(F(":"));
      if(s<10) Serial.print(F("0"));
      Serial.print(s,DEC);
      Serial.println();
    #endif
    editDisplay(1,h,m,s);
  }
}


////////// Hardware outputs //////////

void initOutputs() {
//cf. arduino-clock
}