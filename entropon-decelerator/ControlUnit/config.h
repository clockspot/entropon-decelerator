#pragma once

#define ENABLE_SERIAL_LOGGING //Serial
#define ENABLE_SERIAL_TO_CHAMBER_UNIT //Serial1

//Digital clocks
#define PIN_DIGITAL_ELP_CLK 2
#define PIN_DIGITAL_ELP_DIO 3

#define PIN_DIGITAL_SAV_CLK 4
#define PIN_DIGITAL_SAV_DIO 5

#define PIN_DIGITAL_CHM_CLK 6
#define PIN_DIGITAL_CHM_DIO 7

#define PIN_DIGITAL_NOR_CLK 8 //comment out to disable 7seg displays
#define PIN_DIGITAL_NOR_DIO 9

//Digital clock brightness
#define DIGITAL_NOR_BRIGHTNESS 0x0F
#define DIGITAL_CHM_BRIGHTNESS 0x0A
#define DIGITAL_SAV_BRIGHTNESS 0x0C
#define DIGITAL_ELP_BRIGHTNESS 0x01

#define PIN_POT_MAX_NEG A0 //14
#define PIN_POT_MAX_POS A1 //15
#define PIN_POT_MIN_RATE A2 //16

//If implemented in lieu of wifi connection
// #define PIN_PRINTER_TX 13  //implies SoftwareSerial, but that may not work on SAMD?

//If implemented instead of above
#define PIN_ALT_RELAY 13

//LEDs can be 10/11/12 or 14/15/16 aka A0/A1/A2 depending what needs analog/PWM
#define PIN_LED_STABLE 10 //aka READY in chamber
#define PIN_LED_RECOVERY 11 //aka DONE in chamber
#define PIN_LED_DECEL 12

#define PIN_METER_PWM 17
#define METER_MAX 180

//I2C on 18/19

#define PIN_START_BUTTON 20
#define PIN_START_BUTTON_PRESSED LOW
#define PIN_STOP_BUTTON 21
#define PIN_STOP_BUTTON_PRESSED LOW

// Analog clock and relay pins via PCF8574 I2C expander
#define EXPANDER_ADDRESS 0x20
//the NOR, CHM, SAV analog clocks must be wired to pins 0-5
#define ANALOG_PULSE_WIDTH 35
#define ANALOG_MAX_TICK_RATE 200
//the HV relay must be wired to pin 6
//the LV relay must be wired to pin 7

// #define ENABLE_RTC

// RTC via I2C
//#define RTC_ADDRESS

#define SERIAL_TIMEOUT 100        // milliseconds

#define RATE_MIN 0          // 100 ms per normal second = 90% slower