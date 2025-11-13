#pragma once

// cf. ControlUnit config

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

#define PIN_STOP_BUTTON_INT 10

#define PIN_LED_DECEL 13
#define PIN_LED_RECOVERY 20 //aka DONE in chamber
#define PIN_LED_STABLE 21 //aka READY in chamber

#define PIN_METER_PWM 17
#define METER_MIN 0
#define METER_MAX 217
//going to attempt wiring motors to this via motor driver