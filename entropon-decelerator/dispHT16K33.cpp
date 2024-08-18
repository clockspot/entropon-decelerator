#include <arduino.h>
#include "entropon-decelerator.h"

#ifdef DISPLAY_HT16K33

#include "dispHT16K33.h"
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>

//There will be displays of inside and outside time,
//which may be in multiple locations (inner or outer)

//These are the display hardware
#ifdef HT16K33_INNERDISP_OUTERTIME_ADDR
Adafruit_7segment dispIO = Adafruit_7segment();
#endif
#ifdef HT16K33_INNERDISP_INNERTIME_ADDR
Adafruit_7segment dispII = Adafruit_7segment();
#endif
#ifdef HT16K33_OUTERDISP_OUTERTIME_ADDR
Adafruit_7segment dispOO = Adafruit_7segment();
#endif
#ifdef HT16K33_OUTERDISP_INNERTIME_ADDR
Adafruit_7segment dispOI = Adafruit_7segment();
#endif

void initDisplay() {
  #ifdef HT16K33_INNERDISP_OUTERTIME_ADDR
  dispIO.begin(HT16K33_INNERDISP_OUTERTIME_ADDR);
  dispIO.setBrightness(HT16K33_BRIGHTNESS);
  #endif
  #ifdef HT16K33_INNERDISP_INNERTIME_ADDR
  dispII.begin(HT16K33_INNERDISP_INNERTIME_ADDR);
  dispII.setBrightness(HT16K33_BRIGHTNESS);
  #endif
  #ifdef HT16K33_OUTERDISP_OUTERTIME_ADDR
  dispOO.begin(HT16K33_OUTERDISP_OUTERTIME_ADDR);
  dispOO.setBrightness(HT16K33_BRIGHTNESS);
  #endif
  #ifdef HT16K33_OUTERDISP_INNERTIME_ADDR
  dispOI.begin(HT16K33_OUTERDISP_INNERTIME_ADDR);
  dispOI.setBrightness(HT16K33_BRIGHTNESS);
  #endif
}

void editDisplay(byte which, byte h, byte m, byte s, bool colon) {
  byte v = 0;
  for(byte i=0; i<=6; i++) {
    if(i==0) v = (h<10? 32: h/10); //no leading zero on hour
    if(i==1) v = h%10;
    if(i==2) continue; //reserved for colon //v = s%2; //
    if(i==3) v = m/10;
    if(i==4) v = m%10;
    if(i==5) v = s/10;
    if(i==6) v = s%10;
    if(which==0) { //outer time on all displays
      #ifdef HT16K33_OUTERDISP_OUTERTIME_ADDR
      dispOO.writeDigitNum(i,v,colon);
      #endif
      #ifdef HT16K33_INNERDISP_OUTERTIME_ADDR
      dispIO.writeDigitNum(i,v,colon);
      #endif
    }
    if(which==1) { //inner time on all displays
      #ifdef HT16K33_OUTERDISP_INNERTIME_ADDR
      dispOI.writeDigitNum(i,v,colon);
      #endif
      #ifdef HT16K33_INNERDISP_INNERTIME_ADDR
      dispII.writeDigitNum(i,v,colon);
      #endif
    }
  }
  if(which==0) { //outer time on all displays
    #ifdef HT16K33_OUTERDISP_OUTERTIME_ADDR
    dispOO.writeDisplay();
    #endif
    #ifdef HT16K33_INNERDISP_OUTERTIME_ADDR
    dispIO.writeDisplay();
    #endif
  }
  if(which==1) { //inner time on all displays
    #ifdef HT16K33_OUTERDISP_INNERTIME_ADDR
    dispOI.writeDisplay();
    #endif
    #ifdef HT16K33_INNERDISP_INNERTIME_ADDR
    dispII.writeDisplay();
    #endif
  }
}

#endif //DISPLAY_HT16K33