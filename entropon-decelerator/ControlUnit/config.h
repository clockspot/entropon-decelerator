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

/*
Variable pins

D   A   PWM (to drive motors and meters)
            Int (for immediate button reading)
                AR (for reading pots)
                    Purpose [brackets=fixed]

2       PWM Int     [Digital]
3       PWM Int     [Digital]
4                   [Digital]
5       PWM         [Digital]
6       PWM         [Digital]
7                   [Digital]
8                   [Digital]
9       PWM Int     [Digital]
10      PWM Int     /Btn Start INT/
11      PWM Int     /Btn Stop INT/
12      PWM         (Free - funky relay, printer TX?)

13          Int     /LED Decel On - also onboard LED/
14  0           AR  Pot
15  1       Int AR  Pot
16  2   PWM     AR  Pot
17  3   PWM     AR  [Meter]
18  4           AR  [I2C]
19  5   PWM Int AR  [I2C]
20  6           AR  /LED Recovery/Done/
21  7       Int AR  /LED Stable/Ready/

PCF8574 can generate interrupt when inputs change, so 1 pin for 2 buttons
*/

#define PIN_STOP_BUTTON_INT 10
#define PIN_START_BUTTON_INT 11

// #define PIN_RELAY_DECEL 12

#define PIN_LED_DECEL 13
#define PIN_LED_RECOVERY 20 //aka DONE in chamber
#define PIN_LED_STABLE 21 //aka READY in chamber

#define PIN_POT_DECEL A0 //14 //formerly PIN_POT_MAX_NEG
#define PIN_POT_POWER A1 //15 //formerly PIN_POT_MAX_POS
#define PIN_POT_RECOVERY A2 //16 //formerly MIN_RATE

#define PIN_METER_PWM 17
#define METER_MAX 180

//I2C on 18/19

//non-interrupt-driven button pins
// #define PIN_START_BUTTON 20
// #define PIN_START_BUTTON_PRESSED LOW
// #define PIN_STOP_BUTTON 21
// #define PIN_STOP_BUTTON_PRESSED LOW

// Analog clock and relay pins via PCF8574 I2C expander
#define EXPANDER_ADDRESS 0x20
//the NOR, CHM, SAV analog clocks must be wired to pins 0-5
#define ANALOG_PULSE_WIDTH 35
#define ANALOG_MAX_TICK_RATE 200

#define ENABLE_RTC //I2C

#define SERIAL_TIMEOUT 100        // milliseconds

#define RATE_MIN 0          // 100 ms per normal second = 90% slower

// network and printer
//only wpa supported (cf. other repos for wep support)

// #define NETWORK_SSID "..."
// #define NETWORK_PASS "..."
// #define NETWORK_TRY_NTP
// #define NETWORK_TRY_PRINT
#include "lm-network.h"

#define BOCA_IP_A 192
#define BOCA_IP_B 168
#define BOCA_IP_C 1
#define BOCA_IP_D 244
#define BOCA_IP_PORT 9100