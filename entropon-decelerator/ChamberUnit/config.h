#pragma once

#define ENABLE_SERIAL_LOGGING //Serial
#define ENABLE_SERIAL_TO_CONTROL_UNIT //Serial1

//TODO combine sketches so the chamber can be a regular control panel when a pin is connected at startup

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

//use 10/11/12 for motor(s) if they are PWM?
//#define PIN_VIBES_PWM
// Effect parameters
// #define VIBRATION_MIN_PWM 0
// #define VIBRATION_MAX_PWM 200

//If implemented instead of above
#define PIN_ALT_RELAY 13

#define PIN_LED_STABLE 14 //aka READY in chamber
#define PIN_LED_RECOVERY 15 //aka DONE in chamber
#define PIN_LED_DECEL 16

#define PIN_METER_PWM 17
#define METER_MAX 222

//I2C on 18/19 - n/a for chamber unit

#define PIN_START_BUTTON 20
#define PIN_START_BUTTON_PRESSED LOW
#define PIN_STOP_BUTTON 21
#define PIN_STOP_BUTTON_PRESSED LOW

// // LED strip parameters
// #define NUM_LEDS 60

#define SERIAL_TIMEOUT 100        // milliseconds
