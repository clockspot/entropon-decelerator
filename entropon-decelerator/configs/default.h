//Arduino IoT with LED matrix

#ifndef CONFIG
#define CONFIG


///// Inputs and Outputs /////

#define CTRL_BTN A6


///// Real-Time Clock /////

#define RTC_IS_MILLIS
#define ANTI_DRIFT 0 //msec to add/remove per second - or seconds to add/remove per day divided by 86.4 - to compensate for natural drift. If using wifinina, it really only needs to be good enough for a decent timekeeping display until the next ntp sync. TIP: setting to a superhigh value is helpful for testing! e.g. 9000 will make it run 10x speed


///// Display /////
//If using 4/6-digit 7-segment LED display with HT16K33 (I2C on SDA/SCL pins)
//Requires Adafruit libraries LED Backpack, GFX, and BusIO
//If 6 digits, edit Adafruit_LEDBackpack.cpp to replace "if (d > 4)" with "if (d > 6)"
//and, if desired, in sevensegmentfonttable, replace 0b01111101 with 0b01111100 and 0b01101111 with 0b01100111
//(in previous versions, in numbertable[], replace 0x7D with 0x7C and 0x6F with 0x67)
//to remove the serifs from 6 and 9 for legibility (see http://www.harold.thimbleby.net/cv/files/seven-segment.pdf)
#define DISPLAY_HT16K33
#define HT16K33_BRIGHTNESS 15
#define HT16K33_INNERDISP_OUTERTIME_ADDR 0x70 //0x70 is the default
#define HT16K33_INNERDISP_OUTERTIME_USE2
#define HT16K33_INNERDISP_INNERTIME_ADDR 0x72 //0x70 is the default
// #define HT16K33_OUTERDISP_OUTERTIME_ADDR 0x71 //0x70 is the default
// #define HT16K33_OUTERDISP_INNERTIME_ADDR 0x70 //0x70 is the default

#define LAVET
#define LAVET_OUTERDISP_OUTERTIME_PINEVEN
#define LAVET_OUTERDISP_OUTERTIME_PINODD
#define LAVET_OUTERDISP_INNERTIME_PINEVEN
#define LAVET_OUTERDISP_INNERTIME_PINODD
// #define LAVET_INNERDISP_OUTERTIME_PINEVEN
// #define LAVET_INNERDISP_OUTERTIME_PINODD
// #define LAVET_INNERDISP_INNERTIME_PINEVEN
// #define LAVET_INNERDISP_INNERTIME_PINODD

//a lavet-type control for accumulated time
#define LAVET_ACCUM_PINEVEN
#define LAVET_ACCUM_PINODD


///// Network /////
// #define NETWORK_SSID "..."
// #define NETWORK_PASS "..."
// #include "lm-network.h"


#endif