#pragma once

//Pin definitions
//Hardware inputs
#define PIN_START_BUTTON 4
#define PIN_STOP_BUTTON 5
// #define PIN_POT_MAX_NEG A0
// #define PIN_POT_MAX_POS A1
// #define PIN_POT_MIN_RATE A2

//SoftwareSerial
// #define PIN_CHAMBER_RX 11
// #define PIN_CHAMBER_TX 10
// #define PIN_PRINTER_RX 13 //TODO do we need?
// #define PIN_PRINTER_TX 12

//TODO I2C SDA?
//TODO I2C SCL?
// #define RTC_ENABLED

//DisplayManager
#define PIN_DIGITAL_OUT_DIO 8
#define PIN_DIGITAL_OUT_CLK 9

#define PIN_DIGITAL_CHM_DIO 2
#define PIN_DIGITAL_CHM_CLK 3

#define PIN_DIGITAL_DIF_DIO 10
#define PIN_DIGITAL_DIF_CLK 11

#define PIN_DIGITAL_ELP_DIO 12
#define PIN_DIGITAL_ELP_CLK 13 //blinky!

// #define PIN_LED_NORMAL 4
// #define PIN_LED_DECEL 5
// #define PIN_LED_RECOVERY 6

#define PIN_METER_PWM 6

// #define PIN_ANALOG_OUT_A 16
// #define PIN_ANALOG_OUT_B 17
// #define PIN_ANALOG_CHM_A 18
// #define PIN_ANALOG_CHM_B 19
// #define PIN_ANALOG_DIF_A 20
// #define PIN_ANALOG_DIF_B 21


// Timing parameters
#define CLOCK_PULSE_DURATION 30  // milliseconds
#define SERIAL_TIMEOUT 100        // milliseconds

// Display parameters
#define DISPLAY_UPDATE_INTERVAL 50  // milliseconds

// Constants for rate calculation
#define RATE_NORMAL 1000        // 1000 ms per outside second = normal
#define RATE_MIN 100           // 100 ms per outside second = 90% slower
#define RATE_MAX 1000          // Maximum rate (normal speed)