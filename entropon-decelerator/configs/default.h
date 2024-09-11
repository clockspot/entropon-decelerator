//Arduino IoT with LED matrix

#ifndef CONFIG
#define CONFIG

#define SHOW_SERIAL

///// Inputs and Outputs /////

#define CTRL_BTN A7


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
//0x70 is the default addr
#define DISPLAY_HT16K33
#define HT16K33_BRIGHTNESS 15
// #define HT16K33_INNERDISP_OUTERTIME_ADDR 0x72 //also two extra digits for POWER LEVEL
#define HT16K33_INNERDISP_INNERTIME_ADDR 0x71 //also two extra digits for DIFF
// #define HT16K33_OUTERDISP_OUTERTIME_ADDR 0x71
// #define HT16K33_OUTERDISP_INNERTIME_ADDR 0x71
//removed use2

#define LAVET
#define LAVET_DELAY 40
//pin registers
#define LAVET_OUTERTIME_PINEVEN 2 //D2
#define LAVET_OUTERTIME_PINODD 3 //D3
#define LAVET_INNERTIME_PINEVEN 4 //D4
#define LAVET_INNERTIME_PINODD 5 //D5
#define LAVET_SAVEDTIME_PINEVEN 6 //D6
#define LAVET_SAVEDTIME_PINODD 7 //D7


///// Network /////
//only wpa supported (cf. other repos for wep support)

// #define NETWORK_SSID "..."
// #define NETWORK_PASS "..."
// #define NETWORK_TRY_NTP
// #define NETWORK_TRY_PRINT
#include "lm-network.h"

// printer
#define BOCA_IP_A 192
#define BOCA_IP_B 168
#define BOCA_IP_C 1
#define BOCA_IP_D 244
#define BOCA_IP_PORT 9100


#endif