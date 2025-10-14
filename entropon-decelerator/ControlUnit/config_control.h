#pragma once

//Pin definitions

//Digital clocks
#define PIN_DIGITAL_ELP_CLK 2
#define PIN_DIGITAL_ELP_DIO 3

#define PIN_DIGITAL_DIF_CLK 4
#define PIN_DIGITAL_DIF_DIO 5

#define PIN_DIGITAL_CHM_CLK 6
#define PIN_DIGITAL_CHM_DIO 7

#define PIN_DIGITAL_OUT_CLK 8
#define PIN_DIGITAL_OUT_DIO 9

// #define PIN_POT_MAX_NEG 10
// #define PIN_POT_MAX_POS 11
// #define PIN_POT_MIN_RATE 12

//SoftwareSerial
// #define PIN_CHAMBER_RX normal
// #define PIN_CHAMBER_TX normal
// #define PIN_PRINTER_RX //do we need?
// #define PIN_PRINTER_TX 13

#define PIN_LED_STABLE 14
#define PIN_LED_RECOVERY 15
#define PIN_LED_DECEL 16

#define PIN_METER_PWM 17
#define METER_MAX 188

//I2C should happen on 18/19 as normal

#define PIN_START_BUTTON 20
#define PIN_START_BUTTON_PRESSED LOW
#define PIN_STOP_BUTTON 21
#define PIN_STOP_BUTTON_PRESSED HIGH


// Analog clock pins via PCF8574 I2C expander
#define ANALOG_EXPANDER_ADDRESS 0x20
//the OUT, CHM, DIF analog clocks must be wired to pins 0-5
#define ANALOG_PULSE_WIDTH 35
#define ANALOG_MAX_TICK_RATE 200

//Digital clock brightness
#define DIGITAL_OUT_BRIGHTNESS 0x0F
#define DIGITAL_CHM_BRIGHTNESS 0x0A
#define DIGITAL_DIF_BRIGHTNESS 0x0C
#define DIGITAL_ELP_BRIGHTNESS 0x01


#define SERIAL_TIMEOUT 100        // milliseconds

#define RATE_MIN 0          // 100 ms per outside second = 90% slower