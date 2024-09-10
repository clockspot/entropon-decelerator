#include <arduino.h>
#include "entropon-decelerator.h"

#ifdef DISPLAY_HT16K33

#include "dispHT16K33.h"
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>

//There will be displays of inside and outside time,
//which may be in multiple locations (inner or outer)
//Controllers for outside time also displays power level
//Controllers for inside time also displays seconds saved

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

void displayOuterTime(byte h, byte m, byte s, bool colon) {
  #ifdef HT16K33_OUTERDISP_OUTERTIME_ADDR
  dispOO.writeDigitNum(0, (h<10? 32: h/10), 0);
  dispOO.writeDigitNum(1, h%10, colon);
  dispOO.writeDigitNum(2, m/10, 0);
  dispOO.writeDigitNum(3, m%10, colon);
  dispOO.writeDigitNum(4, s/10, 0);
  dispOO.writeDigitNum(5, s%10, 0);
  dispOO.writeDisplay();
  #endif
  #ifdef HT16K33_INNERDISP_OUTERTIME_ADDR
  dispIO.writeDigitNum(0, (h<10? 32: h/10), 0);
  dispIO.writeDigitNum(1, h%10, colon);
  dispIO.writeDigitNum(2, m/10, 0);
  dispIO.writeDigitNum(3, m%10, colon);
  dispIO.writeDigitNum(4, s/10, 0);
  dispIO.writeDigitNum(5, s%10, 0);
  dispIO.writeDisplay();
  #endif
  //Debugging with Adafruit 1.2" display - outer time secs on first two digits. Hoping one blinks a dot.
  #ifdef HT16K33_INNERDISP_INNERTIME_ADDR
  // dispII.writeDigitNum(0, (h<10? 32: h/10), 0);
  // dispII.writeDigitNum(1, h%10, colon);
  dispII.writeDigitNum(0, s/10, colon);
  dispII.writeDigitNum(1, s%10, colon);
  dispII.writeDisplay();
  #endif
}

void displayInnerTime(byte h, byte m, byte s, bool colon) {
  //TODO change to 0 1 2 3 4 5
  #ifdef HT16K33_OUTERDISP_INNERTIME_ADDR
  dispOI.writeDigitNum(0, (h<10? 32: h/10), 0);
  dispOI.writeDigitNum(1, h%10, colon);
  dispOI.writeDigitNum(2, m/10, 0);
  dispOI.writeDigitNum(3, m%10, colon);
  dispOI.writeDigitNum(4, s/10, 0);
  dispOI.writeDigitNum(5, s%10, 0);
  dispOI.writeDisplay();
  #endif
  // #ifdef HT16K33_INNERDISP_INNERTIME_ADDR
  // dispII.writeDigitNum(0, (h<10? 32: h/10), 0);
  // dispII.writeDigitNum(1, h%10, colon);
  // dispII.writeDigitNum(2, m/10, 0);
  // dispII.writeDigitNum(3, m%10, colon);
  // dispII.writeDigitNum(4, s/10, 0);
  // dispII.writeDigitNum(5, s%10, 0);
  // dispII.writeDisplay();
  // #endif
  //Debugging with Adafruit 1.2" display - inner time secs on second two digits.
  #ifdef HT16K33_INNERDISP_INNERTIME_ADDR
  dispII.drawColon(colon);
  dispII.writeDigitNum(3, s/10, 0);
  dispII.writeDigitNum(4, s%10, 0);
  dispII.writeDisplay();
  #endif
}

void displayPowerLevel(byte p) {
  //Controllers for inside time also display power level
  #ifdef HT16K33_OUTERDISP_OUTERTIME_ADDR
  dispOO.writeDigitNum(6, (p<10? 32: p/10), 0); //TODO leading zero or nah?
  dispOO.writeDigitNum(7, p%10, 0); //TODO could decimal to indicate power state or such
  dispOO.writeDisplay();
  #endif
  #ifdef HT16K33_INNERDISP_OUTERTIME_ADDR
  dispIO.writeDigitNum(6, (p<10? 32: p/10), 0);
  dispIO.writeDigitNum(7, p%10, 0);
  dispIO.writeDisplay();
  #endif
}

void displaySession(byte s) {
  //Translates from session stage to state
  switch(s) {
    case 0: default: //normal, clocks match
      displayState(32,32,1,1); //".."
      break;
    case 1: //slow
      displayState(79,110); //"On"
      break;
    case 2: //normal, inner clock is slow
      displayState(45,45); //"--"
      break;
    case 3: //fast forward
      displayState(70,70); //"FF"
      break;
  }
}

void displayState(char a, char b, bool dotA, bool dotB) {
  //Alternate use of power level display:
  //Controllers for inside time also display state
  #ifdef HT16K33_OUTERDISP_OUTERTIME_ADDR
  dispOO.writeDigitAscii(6, a, dotA); //TODO leading zero or nah?
  dispOO.writeDigitAscii(7, b, dotB); //TODO could decimal to indicate power state or such
  dispOO.writeDisplay();
  #endif
  #ifdef HT16K33_INNERDISP_OUTERTIME_ADDR
  dispIO.writeDigitAscii(6, a, dotA); //TODO leading zero or nah?
  dispIO.writeDigitAscii(7, b, dotB); //TODO could decimal to indicate power state or such
  dispIO.writeDisplay();
  #endif
  //Debugging with Adafruit 1.2" display - inner time secs on second two digits.
  #ifdef HT16K33_INNERDISP_INNERTIME_ADDR
  dispII.writeDigitAscii(5, a, dotA); //TODO leading zero or nah?
  dispII.writeDigitAscii(6, b, dotB); //TODO could decimal to indicate power state or such
  dispII.writeDisplay();
  #endif
}

void displaySecondsSaved(byte s) {
  //Controllers for inside time also displays seconds saved
  //TODO reenable
  // #ifdef HT16K33_OUTERDISP_INNERTIME_ADDR
  // dispOI.writeDigitNum(6, (s<10? 32: s/10), 0); //TODO leading zero or nah?
  // dispOI.writeDigitNum(7, s%10, 0);
  // dispOI.writeDisplay();
  // #endif
  // #ifdef HT16K33_INNERDISP_INNERTIME_ADDR
  // dispII.writeDigitNum(6, (s<10? 32: s/10), 0);
  // dispII.writeDigitNum(7, s%10, 0);
  // dispII.writeDisplay();
  // #endif
}

#endif //DISPLAY_HT16K33