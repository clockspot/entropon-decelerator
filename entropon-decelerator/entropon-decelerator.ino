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
  setClock(20,0,0);
  // initOutputs(); //depends on some EEPROM settings
  initInputs();
  // initNetwork();
  updateTime();
} //end setup()

void loop(){
  // //checkRTC(false); //if clock has ticked, decrement timer if running, and updateDisplay
  // millisApplyDrift();
  checkInputs(); //if inputs have changed, this will do things + updateDisplay as needed
  // #ifdef NETWORK_H
  //   cycleNetwork();
  // #endif
  // cycleTimer();
  // cycleDisplay();
  // cycleSignal();
  // updateTime(); //TODO repair before reenabling
}



////////// Input handling and value setting //////////

byte inputLast = 0;
unsigned long inputTimeLast = 0;

void initInputs() {
  #ifdef CTRL_BTN
  pinMode(CTRL_BTN, INPUT_PULLUP);
  #endif
}
void checkInputs() {
  if(inputLast) { //Was something pressed? Check if it's been let go
    if(millis()-inputTimeLast > 100) { //debounce
      if(!readBtn(inputLast)) {
        #ifdef SHOW_SERIAL
          Serial.println(F("Big button released"));
        #endif
        inputTimeLast = millis();
        inputLast = 0; //record the button having been let go
      }
    }
  } else { //See if something new is pressed
    #ifdef CTRL_BTN
    if(readBtn(CTRL_BTN) && !inputLast) {
      #ifdef SHOW_SERIAL
        Serial.println(F("Big button pressed"));
      #endif
      inputLast = CTRL_BTN;
      inputTimeLast = millis();
      cycleSession();
    }
    #endif
  }
}
bool readBtn(byte btn) {
  if(btn==A6 || btn==A7) return analogRead(btn)<100; //analog-only pins
  else return !(digitalRead(btn)); //false (low) when pressed
}

////////// Timing and timed events //////////

unsigned long timeOffset = 0; //converts millis() to real time of day in ms (continually adjusted vs. rtc)
unsigned long timeStart = 0; //the real time at which the last entroponics session started
unsigned long timeOuterLast = 0; //the outer (real) clock time, at last tick
unsigned long timeInnerLast = 0; //the inner (fake) clock time, at last tick
unsigned long timeInnerLastTick = 0; //the real time at which the inner clock last ticked
int innerTick = 1000; //current length of inner clock ticks, in real time ms

byte sessionStage = 0; //0 = catchup/normal, 1 = slowing, 2 = steady slow
void cycleSession() {
  switch(sessionStage) {
    case 0: //catchup/normal to slowing
      sessionStage = 1;
      innerTick = 1200;
      #ifdef SHOW_SERIAL
        Serial.println(F("Inner clock slowing to 1200ms"));
      #endif
      break;
    case 1: //slowing to steady slow
      sessionStage = 2;
      innerTick = 1000;
      #ifdef SHOW_SERIAL
        Serial.println(F("Inner clock stabilizing at 1000ms"));
      #endif
      break;
    default: //steady slow to catchup/normal
      sessionStage = 0;
      innerTick = 500;
      #ifdef SHOW_SERIAL
        Serial.println(F("Inner clock speeding to 500ms"));
      #endif
      break;
  }
}

void setClock(byte h, byte m, byte s) {
  //e.g. if millis is 50000 but tod is 0:00:03 (3000), offset is -47000. millis 55000 + offset = tod 0:00:08
  //e.g. (loop 200k) millis 50000, tod 3000, offset is 153000. 55000+153000=208000
  //e.g. if millis is 3000 but tod is 0:00:50 (50000), offset is 47000. millis 6000 + offset = tod 0:00:53
  timeOffset = ((h*3600000)+(m*60000)+(s*1000)) - millis();
}

void updateTime(bool force) {
  unsigned long now = millis()+timeOffset;
  //Has there been an outer (real) time tick? (always 1000ms)
  if(force || now-timeOuterLast > 1000) {
    //todo modify this to catch colon changes
    if(force) timeOuterLast = now;
    else timeOuterLast += 1000;
    //unlike with inner time, outer time is real time, so we can derive display directly from real timestamps
    byte h = (timeOuterLast%86400000)/3600000;
    byte m = (timeOuterLast%3600000)/60000;
    byte s = (timeOuterLast%60000)/1000;
    #ifdef SHOW_SERIAL
      if(force) Serial.print(F("Outer reset at "));
      else Serial.print(F("Outer tick at "));
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
  //Has there been an inner (fake) time tick? (variable length)
  if(force || now-timeInnerLastTick > innerTick) {
    //todo modify this to catch colon changes
    if(force) timeInnerLastTick = now;
    else {
      timeInnerLastTick += innerTick; //real time
      timeInnerLast += 1000; //fake time
    }
    if(sessionStage==0 && innerTick!=1000) {
      //if we're catching up, let's see if we've caught up yet
      //since the times are unsigned, the way to tell if fake has outstripped real is to detect a rollover (big number becomes tiny) - it will probably be exactly 0 but just in case
      if(timeInnerLast - timeOuterLast < 10) {
        timeInnerLast = timeOuterLast;
        timeInnerLastTick = timeOuterLast;
        innerTick = 1000;
        #ifdef SHOW_SERIAL
          Serial.println(F("Inner clock has caught up"));
        #endif
      }
    }
    byte h = (timeInnerLast%86400)/3600;
    byte m = (timeInnerLast%3600)/60;
    byte s = (timeInnerLast%60)/1;
    #ifdef SHOW_SERIAL
      if(force) Serial.print(F("Inner reset at "));
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