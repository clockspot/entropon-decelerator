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
  // setClock(20,0,0); //TODO restore
  // initOutputs(); //depends on some EEPROM settings
  initInputs();
  // initNetwork();
  updateTime(true);
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
  updateTime();
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
    if(millis()-inputTimeLast > 150) { //debounce
      if(!readBtn(inputLast)) {
        #ifdef SHOW_SERIAL
          //Serial.println(F("Big button released"));
        #endif
        inputTimeLast = millis();
        inputLast = 0; //record the button having been let go
      }
    }
  } else { //See if something new is pressed
    if(millis()-inputTimeLast > 150) {
      #ifdef CTRL_BTN
      if(readBtn(CTRL_BTN) && !inputLast) {
        #ifdef SHOW_SERIAL
          //Serial.println(F("Big button pressed"));

        #endif
        inputLast = CTRL_BTN;
        inputTimeLast = millis();
        cycleSession();
      }
      #endif
    }
  }
}
bool readBtn(byte btn) {
  if(btn==A6 || btn==A7) return analogRead(btn)<100; //analog-only pins
  else return !(digitalRead(btn)); //false (low) when pressed
}

////////// Timing and timed events //////////

unsigned long timeOffset = 0; //converts millis() to real time of day in ms (continually adjusted vs. rtc)
unsigned long timeStart = 0; //the real time at which the last entroponics session started
//there is no timeOuter because the outer clocks keep real time, per now/millis
unsigned long timeLast = 0;
unsigned long timeOuterLastTick = 0; //the outer clocks' last tick
unsigned long timeInner = 0; //the inner (fake) clock time
unsigned long timeInnerLastTick = 0; //the inner clocks' last tick
int innerTick = 1000; //current length of inner clock ticks, in real time ms
byte sessionStage = 0; //0 = normal, 1 = slowing, 2 = steady slow, 3 = catching up

//For the purposes of this project,
//"time" is milliseconds since midnight.
//So we need this fn to get time from millis, offset per RTC
unsigned long getTimeNow() {
  return millis(); //+timeOffset;
}
//And we need this one to be able to do math that rolls over as expected
unsigned long getTimeDiff(unsigned long a, unsigned long b) {
  //86399k-86395k = 4k
  //2k-86395k = 7k
  //85300k-86300k = 86400k-1000k = 85400k
  return ((b>a? a+86400000: a)-b);
}

void cycleSession() {
  unsigned long now = millis()+timeOffset;
  switch(sessionStage) {
    case 0: //catchup/normal to slowing
      sessionStage = 1;
      innerTick = 1600;
      timeStart = now;
      #ifdef SHOW_SERIAL
        Serial.println();
        Serial.println(F("Session started. Slowing."));
      #endif
      break;
    case 1: //slowing to steady slow
      sessionStage = 2;
      innerTick = 1000;
      #ifdef SHOW_SERIAL
        Serial.println();
        Serial.println(F("Session finished. Stabilizing."));
        Serial.print(F("The patient spent "));
        Serial.print((now-timeStart)/1000,DEC),
        Serial.print(F(" seconds to save "));
        Serial.print((now-timeInnerLast)/1000,DEC);
        Serial.println(F(" seconds."));
      #endif
      break;
    default: //steady slow to catchup/normal
      sessionStage = 0;
      innerTick = 400;
      //pretend the tick happened per shorter timing
      timeInnerLastTick += (1000-innerTick);
      //TODO need a thing here?
      #ifdef SHOW_SERIAL
        Serial.println();
        Serial.println(F("Catching up."));
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
  unsigned long timeNow = getTimeNow();

  //outer clock
  //look for ticks
  if(force || (timeNow-timeOuterLastTick > 500)) { //check for half tick, which modifies colon
    bool colon = 0;
    if(force || (timeNow-timeOuterLastTick > 1000)) { //check for full tick, which modifies time
      colon = 1;
      if(force) timeOuterLastTick = now;
      else timeOuterLastTick = (now/1000)*1000; //+= 1000;
      //TODO lavet pulse
    } //end full tick
    //unlike with inner time, outer time is real time, so we can derive display directly from real timestamps
    #ifdef DISPLAY_MILS //minutes, seconds, and mils
    editDisplay(0,(timeNow%3600000)/60000,(timeNow%60000)/1000,(now%1000)/10,colon);
    #else
    editDisplay(0,(now%86400000)/3600000,(now%3600000)/60000,(now%60000)/1000,colon);
    #endif
  }
  //inner clock
  //TODO move the inner clock forward incrementally - will need to handle rollovers
  //look for tickets
  if(force || (timeNow-timeInnerLastTick > innerTick/2)) { //check for half tick, which modifies colon
    bool colon = 0;
    if(force || ((now-timeInnerLastTick > innerTick) && (now-timeInnerLastTick < 10000))) { //check for full tick, which modifies time
      colon = 1;
      if(force) {
        timeInnerLastTick = now; //real time
        timeInnerLast = now; //fake time
      } else {
        timeInnerLastTick += innerTick; //real time
        timeInnerLast += 1000; //fake time
      }
      // Serial.println(now-timeInnerLastTick,DEC);
      if(sessionStage==0 && innerTick!=1000) {
        //if we're catching up, let's see if we've caught up yet
        //since the times are unsigned, the way to tell if fake has outstripped real is to detect a rollover (big number becomes small)
        //but we don't want to call it caught up until the next outer tick is further away than the next inner tick
        //so that if we're using lavet steppers, they can stabilize
        #ifdef SHOW_SERIAL
          //Serial.print(F("Inner clock diff: "));
          //Serial.println(timeInnerLast - timeOuterLastTick,DEC);
        #endif

        //(1000 - (now - timeOuterLastTick > 1000? 1000: now - timeOuterLastTick)) > innerTick

        //if we've caught up, and also caught our breath (next outer tick is further away than next fast inner tick)
        if((timeInnerLast - timeOuterLastTick < 10000) && (now-timeOuterLastTick < 1000-innerTick)) {
          timeInnerLast = timeOuterLastTick;
          timeInnerLastTick = timeOuterLastTick;
          innerTick = 1000;
          #ifdef SHOW_SERIAL
            Serial.println();
            Serial.println(F("Caught up."));
          #endif
        }
        /*

        outerTick = 1000
        innerTick = 400

        if lastOuterTick was 100 ago, nextOuterTick is 900 which is greater than 400, so ok
        if lastOuterTick was 500 ago, nextOuterTick is 500 which is greater so ok
        if lastOuterTick was 700 ago, nextOuterTick is 300 which is greater than 400 so not ok
        lastOuterTick needs to be less than 1000-400

        06
            09
        07
        08
            10
        09
        10
        11  11  

        06
            09
        07
        08
            10
        09
        10* - tick diff is 800, we're not done, needs to be less than 
            11
        11! - tick diff is 200, we're done
        12  12

        07
            09
        08
        09
            10
        10! - tick diff is 200, 
        11 11
        11
        
        */

      } //end catchup
      unsigned long diff = timeOuterLastTick-timeInnerLast; //ehhh
      editDisplay(2,(diff%86400000)/3600000,(diff%3600000)/60000,(diff%60000)/1000,1);
    } //end full tick
    editDisplay(1,(timeInnerLast%86400000)/3600000,(timeInnerLast%3600000)/60000,(timeInnerLast%60000)/1000,colon);
  }
}


////////// Hardware outputs //////////

void initOutputs() {
//cf. arduino-clock
}