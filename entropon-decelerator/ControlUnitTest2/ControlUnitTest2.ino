// ==========================================
// PCF8574 + TM1637 Display Test Sketch
// Tests wiring of 4 six-digit displays via I/O expander
// ==========================================

#include <Wire.h>
#include <PCF8574.h>

// PCF8574 I2C address (adjust if needed)
// A2=0, A1=0, A0=0 → 0x20
#define PCF8574_ADDRESS 0x20

// Create PCF8574 instance
PCF8574 expander(PCF8574_ADDRESS);

// TM1637 protocol constants
#define TM1637_CMD1 0x40  // Data command
#define TM1637_CMD2 0xC0  // Address command
#define TM1637_CMD3 0x88  // Display control (on)
#define TM1637_BRIGHTNESS 0x0F  // Max brightness

// Timing delay in microseconds
#define TM1637_DELAY 20

// Pin assignments on PCF8574
// Display 1: CLK=P7, DIO=P6
// Display 2: CLK=P5, DIO=P4
// Display 3: CLK=P3, DIO=P2
// Display 4: CLK=P1, DIO=P0
const uint8_t displayPins[4][2] = {
    {6, 7},  // Display 1: CLK, DIO
    {4, 5},  // Display 2: CLK, DIO
    {2, 3},  // Display 3: CLK, DIO
    {0, 1}   // Display 4: CLK, DIO
};

// Segment patterns for digits 0-9
const uint8_t digitSegments[10] = {
    0x3F,  // 0: 0b00111111
    0x06,  // 1: 0b00000110
    0x5B,  // 2: 0b01011011
    0x4F,  // 3: 0b01001111
    0x66,  // 4: 0b01100110
    0x6D,  // 5: 0b01101101
    0x7D,  // 6: 0b01111101
    0x07,  // 7: 0b00000111
    0x7F,  // 8: 0b01111111
    0x6F   // 9: 0b01101111
};

// Brightness levels for each display
const uint8_t displayBrightness[4] = {
    0x0F,  // Display 1: Max brightness (15/15)
    0x08,  // Display 2: A little dimmer (10/15)
    0x0F,  // Display 3: Max brightness (15/15)
    0x01   // Display 4: Much dimmer (4/15)
};

void updateDisplays(uint8_t out, uint8_t chm, uint8_t dif, uint8_t elp) {

    //start
    expander.selectAll(); //sets all pins HIGH
    expander.selectNone(); //sets all pins LOW

    // #define TM1637_CMD1 0x40  // Data command
    // #define TM1637_CMD2 0xC0  // Address command
    // #define TM1637_CMD3 0x88  // Display control (on)
    // #define TM1637_BRIGHTNESS 0x0F  // Max brightness

    //    dio clk
    //out   7   6
    //chm   5   4
    //dif   3   2
    //elp   1   0

    //write a byte: data command
    // expander.toggleMask(0b00000000); //but all clk pins are low already?
    for(uint8_t i=0; i<8; i++) {
        //we're looking at the eight bits of this value
        if((myNumber >> n) & 1)
        expander.toggleMask()
    }

}

// Function to send start condition
void tm1637_start(uint8_t clkPin, uint8_t dioPin) {
    expander.write(dioPin, HIGH);
    expander.write(clkPin, HIGH);
    delayMicroseconds(TM1637_DELAY);
    expander.write(dioPin, LOW);
    expander.write(clkPin, LOW);
    delayMicroseconds(TM1637_DELAY);
}

// Function to send stop condition
void tm1637_stop(uint8_t clkPin, uint8_t dioPin) {
    expander.write(clkPin, LOW);
    expander.write(dioPin, LOW);
    delayMicroseconds(TM1637_DELAY);
    expander.write(clkPin, HIGH);
    expander.write(dioPin, HIGH);
    delayMicroseconds(TM1637_DELAY);
}

// Function to write a byte
bool tm1637_writeByte(uint8_t clkPin, uint8_t dioPin, uint8_t data) {
    // Write 8 bits
    for (uint8_t i = 0; i < 8; i++) {
        expander.write(clkPin, LOW);
        delayMicroseconds(TM1637_DELAY);
        
        // Write bit (LSB first)
        expander.write(dioPin, (data & 0x01) ? HIGH : LOW);
        data >>= 1;
        
        expander.write(clkPin, HIGH);
        delayMicroseconds(TM1637_DELAY);
    }
    
    // Wait for ACK
    expander.write(clkPin, LOW);
    expander.write(dioPin, HIGH);  // Release DIO for ACK
    delayMicroseconds(TM1637_DELAY);
    
    expander.write(clkPin, HIGH);
    delayMicroseconds(TM1637_DELAY);
    
    bool ack = (expander.read(dioPin) == LOW);
    
    expander.write(clkPin, LOW);
    delayMicroseconds(TM1637_DELAY);
    
    return ack;
}

// Function to send segments to display with reversed order
void sendSegmentsToDisplay(uint8_t displayNum, uint8_t segments[6]) {
    uint8_t clkPin = displayPins[displayNum][0];
    uint8_t dioPin = displayPins[displayNum][1];
    
    // Reverse the segment order (swap first 3 with last 3)
    uint8_t reversedSegments[6];
    reversedSegments[0] = segments[3];
    reversedSegments[1] = segments[4];
    reversedSegments[2] = segments[5];
    reversedSegments[3] = segments[0];
    reversedSegments[4] = segments[1];
    reversedSegments[5] = segments[2];
    
    // Send Command1 - Data command
    tm1637_start(clkPin, dioPin);
    tm1637_writeByte(clkPin, dioPin, TM1637_CMD1);
    tm1637_stop(clkPin, dioPin);
    
    // Send Command2 - Address command + data
    tm1637_start(clkPin, dioPin);
    tm1637_writeByte(clkPin, dioPin, TM1637_CMD2);
    
    // Write 6 bytes of segment data (reversed)
    for (uint8_t i = 0; i < 6; i++) {
        tm1637_writeByte(clkPin, dioPin, reversedSegments[i]);
    }
    tm1637_stop(clkPin, dioPin);
    
    // Send Command3 - Display control with specific brightness
    tm1637_start(clkPin, dioPin);
    tm1637_writeByte(clkPin, dioPin, TM1637_CMD3 | 0x08 | displayBrightness[displayNum]);
    tm1637_stop(clkPin, dioPin);
}

// Function to display 6 digits on a display
void displayNumber(uint8_t displayNum, uint8_t digit) {
    // Prepare 6 digits of the same number
    uint8_t segments[6];
    for (uint8_t i = 0; i < 6; i++) {
        segments[i] = digitSegments[digit];
    }
    
    sendSegmentsToDisplay(displayNum, segments);
}

// Function to clear a display
void clearDisplay(uint8_t displayNum) {
    uint8_t segments[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    sendSegmentsToDisplay(displayNum, segments);
}

// ==========================================
// Main Setup and Loop
// ==========================================

void setup() {
    Serial.begin(115200);
    Serial.println(F("\n================================="));
    Serial.println(F("PCF8574 + TM1637 Display Test"));
    Serial.println(F("=================================\n"));
    
    // Initialize I2C
    Wire.begin();
    Serial.println(F("I2C initialized"));
    
    // Check if PCF8574 is responding
    Wire.beginTransmission(PCF8574_ADDRESS);
    uint8_t error = Wire.endTransmission();
    
    if (error == 0) {
        Serial.print(F("PCF8574 found at address 0x"));
        Serial.println(PCF8574_ADDRESS, HEX);
    } else {
        Serial.print(F("ERROR: PCF8574 not found at address 0x"));
        Serial.print(PCF8574_ADDRESS, HEX);
        Serial.println(F("\nCheck wiring and address jumpers!"));
        Serial.println(F("A2=A1=A0=GND → 0x20"));
        Serial.println(F("A0=VCC, A1=A2=GND → 0x21"));
        Serial.println(F("etc..."));
        
        // Scan for PCF8574 on all addresses
        Serial.println(F("\nScanning for PCF8574..."));
        for (uint8_t addr = 0x20; addr <= 0x27; addr++) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
                Serial.print(F("Found device at 0x"));
                Serial.println(addr, HEX);
            }
        }
        
        while(1); // Stop here
    }
    
    // Initialize PCF8574
    expander.begin();
    Serial.println(F("PCF8574 initialized"));
    
    // Set all pins HIGH initially (idle state for TM1637)
    expander.selectAll();
    delay(100);

    updateDisplays(0,0,0,0);
}

void loop() {
    static uint8_t testMode = 0;
    static unsigned long lastChange = 0;
    
    // Change pattern every 3 seconds
    if (millis() - lastChange > 3000) {
        lastChange = millis();
        testMode++;
        
        if (testMode > 3) testMode = 0;
        
        switch(testMode) {
            case 0:
                Serial.println(F("Test: Individual digits (1111, 2222, 3333, 4444)"));
                updateDisplays(111111,222222,333333,444444);
                break;
                
            case 1:
                Serial.println(F("Test: All 8s (888888 on all displays)"));
                updateDisplays(888888,888888,888888,888888);
                break;
                
            case 2:
                Serial.println(F("Test: Sequential (123456 pattern)"));
                updateDisplays(123456,123456,123456,123456);
                break;
                
            case 3:
                Serial.println(F("Test: Clear all displays"));
                updateDisplays()
                for (uint8_t i = 0; i < 4; i++) {
                    clearDisplay(i);
                }
                break;
        }
    }
}

// ==========================================
// Troubleshooting Guide (in comments)
// ==========================================

/*
TROUBLESHOOTING:

1. PCF8574 Not Found:
   - Check I2C connections (SDA to A4, SCL to A5)
   - Verify pull-up resistors (4.7kΩ) on SDA and SCL
   - Check PCF8574 power (VCC to 5V, GND to GND)
   - Verify address jumpers (A0, A1, A2)

2. Display Not Working:
   - Check display power (5V and GND)
   - Verify CLK and DIO connections to correct PCF8574 pins
   - Try swapping CLK and DIO if display is completely dark
   - Check for loose connections

3. Garbled Display:
   - May indicate timing issues - try increasing TM1637_DELAY
   - Check for adequate power supply current
   - Verify I2C bus speed isn't too high

4. Dim Display:
   - Normal - brightness is set to maximum (0x0F)
   - Can be adjusted by changing TM1637_BRIGHTNESS

PCF8574 Pin Mapping:
   P7 (pin 4) → Display 1 CLK
   P6 (pin 5) → Display 1 DIO
   P5 (pin 6) → Display 2 CLK
   P4 (pin 7) → Display 2 DIO
   P3 (pin 9) → Display 3 CLK
   P2 (pin 10) → Display 3 DIO
   P1 (pin 11) → Display 4 CLK
   P0 (pin 12) → Display 4 DIO

Physical IC Pinout (DIP-16):
   1  - A0 (address)
   2  - A1 (address)
   3  - A2 (address)
   4  - P0
   5  - P1
   6  - P2
   7  - P3
   8  - GND
   9  - P4
   10 - P5
   11 - P6
   12 - P7
   13 - INT (not used)
   14 - SCL
   15 - SDA
   16 - VCC
*/