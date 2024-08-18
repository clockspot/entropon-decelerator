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
#ifdef HT16K33_INNERDISP_DIFF_ADDR
Adafruit_7segment dispID = Adafruit_7segment();
#endif
#ifdef HT16K33_OUTERDISP_OUTERTIME_ADDR
Adafruit_7segment dispOO = Adafruit_7segment();
#endif
#ifdef HT16K33_OUTERDISP_INNERTIME_ADDR
Adafruit_7segment dispOI = Adafruit_7segment();
#endif
#ifdef HT16K33_OUTERDISP_DIFF_ADDR
Adafruit_7segment dispOD = Adafruit_7segment();
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
  #ifdef HT16K33_INNERDISP_DIFF_ADDR
  dispID.begin(HT16K33_INNERDISP_DIFF_ADDR);
  dispID.setBrightness(HT16K33_BRIGHTNESS);
  #endif
  #ifdef HT16K33_OUTERDISP_OUTERTIME_ADDR
  dispOO.begin(HT16K33_OUTERDISP_OUTERTIME_ADDR);
  dispOO.setBrightness(HT16K33_BRIGHTNESS);
  #endif
  #ifdef HT16K33_OUTERDISP_INNERTIME_ADDR
  dispOI.begin(HT16K33_OUTERDISP_INNERTIME_ADDR);
  dispOI.setBrightness(HT16K33_BRIGHTNESS);
  #endif
  #ifdef HT16K33_OUTERDISP_DIFF_ADDR
  dispOD.begin(HT16K33_OUTERDISP_DIFF_ADDR);
  dispOD.setBrightness(HT16K33_BRIGHTNESS);
  #endif
}

void editDisplay(byte which, byte h, byte m, byte s, bool colon) {
  bool use2 = 0;
  if(which==0) { //outer time on all displays
    #ifdef HT16K33_OUTERDISP_OUTERTIME_ADDR
    #ifdef HT16K33_OUTERDISP_OUTERTIME_USE2
    use2 = 1;
    #else
    use2 = 0;
    dispOO.drawColon(colon);
    #endif
    dispOO.writeDigitNum(0, (h<10? 32: h/10), 0);
    dispOO.writeDigitNum(1, h%10, colon);
    dispOO.writeDigitNum(2+!use2, m/10, 0);
    dispOO.writeDigitNum(3+!use2, m%10, colon);
    dispOO.writeDigitNum(5, s/10, 0);
    dispOO.writeDigitNum(6, s%10, 0);
    dispOO.writeDisplay();
    #endif
    #ifdef HT16K33_INNERDISP_OUTERTIME_ADDR
    #ifdef HT16K33_INNERDISP_OUTERTIME_USE2
    use2 = 1;
    #else
    use2 = 0;
    dispIO.drawColon(colon);
    #endif
    dispIO.writeDigitNum(0, (h<10? 32: h/10), 0);
    dispIO.writeDigitNum(1, h%10, colon);
    dispIO.writeDigitNum(2+!use2, m/10, 0);
    dispIO.writeDigitNum(3+!use2, m%10, colon);
    dispIO.writeDigitNum(5, s/10, 0);
    dispIO.writeDigitNum(6, s%10, 0);
    dispIO.writeDisplay();
    #endif
  }
  if(which==1) { //inner time on all displays
    #ifdef HT16K33_OUTERDISP_INNERTIME_ADDR
    #ifdef HT16K33_OUTERDISP_INNERTIME_USE2
    use2 = 1;
    #else
    use2 = 0;
    dispOI.drawColon(colon);
    #endif
    dispOI.writeDigitNum(0, (h<10? 32: h/10), 0);
    dispOI.writeDigitNum(1, h%10, colon);
    dispOI.writeDigitNum(2+!use2, m/10, 0);
    dispOI.writeDigitNum(3+!use2, m%10, colon);
    dispOI.writeDigitNum(5, s/10, 0);
    dispOI.writeDigitNum(6, s%10, 0);
    dispOI.writeDisplay();
    #endif
    #ifdef HT16K33_INNERDISP_INNERTIME_ADDR
    #ifdef HT16K33_INNERDISP_INNERTIME_USE2
    use2 = 1;
    #else
    use2 = 0;
    dispII.drawColon(colon);
    #endif
    dispII.writeDigitNum(0, (h<10? 32: h/10), 0);
    dispII.writeDigitNum(1, h%10, colon);
    dispII.writeDigitNum(2+!use2, m/10, 0);
    dispII.writeDigitNum(3+!use2, m%10, colon);
    dispII.writeDigitNum(5, s/10, 0);
    dispII.writeDigitNum(6, s%10, 0);
    dispII.writeDisplay();
    #endif
  }
  if(which==2) { //diff on all displays
    #ifdef HT16K33_OUTERDISP_DIFF_ADDR
    #ifdef HT16K33_OUTERDISP_DIFF_USE2
    use2 = 1;
    #else
    use2 = 0;
    dispOD.drawColon(colon);
    #endif
    dispOD.writeDigitNum(0, (h<10? 32: h/10), 0);
    dispOD.writeDigitNum(1, h%10, colon);
    dispOD.writeDigitNum(2+!use2, m/10, 0);
    dispOD.writeDigitNum(3+!use2, m%10, colon);
    dispOD.writeDigitNum(5, s/10, 0);
    dispOD.writeDigitNum(6, s%10, 0);
    dispOD.writeDisplay();
    #endif
    #ifdef HT16K33_INNERDISP_DIFF_ADDR
    #ifdef HT16K33_INNERDISP_DIFF_USE2
    use2 = 1;
    #else
    use2 = 0;
    dispID.drawColon(colon);
    #endif
    dispID.writeDigitNum(0, (h<10? 32: h/10), 0);
    dispID.writeDigitNum(1, h%10, colon);
    dispID.writeDigitNum(2+!use2, m/10, 0);
    dispID.writeDigitNum(3+!use2, m%10, colon);
    dispID.writeDigitNum(5, s/10, 0);
    dispID.writeDigitNum(6, s%10, 0);
    dispID.writeDisplay();
    #endif
  }
}

#endif //DISPLAY_HT16K33