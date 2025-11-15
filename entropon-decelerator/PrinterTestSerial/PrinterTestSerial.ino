#include "config.h"

void setup() {
  delay(2000);
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Hello world");

  // Serial1.begin(9600);
  // pinMode(A4,OUTPUT);
  // Serial2.begin(9600);

  Serial3.begin(9600);

  // // Assign pins to SERCOM1
  // pinPeripheral(20, PIO_SERCOM);   // RX (even if unused)  
  // pinPeripheral(21, PIO_SERCOM);  // TX
  
  // Wait for sync
  while(SERCOM3->USART.SYNCBUSY.bit.ENABLE);
  
  // Disable SERCOM
  SERCOM3->USART.CTRLA.bit.ENABLE = 0;
  while(SERCOM3->USART.SYNCBUSY.bit.ENABLE);
  
  // Get current CTRLA value and set TXINV bit
  uint32_t ctrla = SERCOM3->USART.CTRLA.reg;
  ctrla |= (1 << 8);  // TXINV is bit 8
  SERCOM3->USART.CTRLA.reg = ctrla;
  
  // Re-enable SERCOM
  SERCOM3->USART.CTRLA.bit.ENABLE = 1;
  while(SERCOM3->USART.SYNCBUSY.bit.ENABLE);
  
  delay(1000);

}

void loop() {
  if(Serial.available()) {
    char incomingChar = Serial.read();
    if(incomingChar >= '0' && incomingChar <= '9') {
      int digit = incomingChar - '0';
      Serial.print("Command: ");
      Serial.println(digit);
      printCertificate(digit);
    } else {
      Serial.println("Unrecognized command");
    }
    while(Serial.available()) Serial.read();
  }
}

void printCertificate(int digit) {
  Serial.println(F("Printing now"));
  // Serial1.print(digit);
  // Serial1.println("Hello from Arduino!");
  // Serial1.write(0x0C);
  Serial3.println("Hello from Arduino!");
  Serial3.write(0x0C);

  // delay(1000);
    
  // // Send specific bytes to see pattern
  // Serial1.write(0x41);  // 'A' = 65
  // delay(10);
  // Serial1.write(0x42);  // 'B' = 66
  // delay(10);
  // Serial1.write(0x43);  // 'C' = 67
  // delay(10);
  // Serial1.write(0x0D);  // CR = 13
  // delay(10);
  // Serial1.write(0x0A);  // LF = 10
  // delay(10);
  // Serial1.write(0x0C);  // FF = 12


  // Serial1.print(F("<RC10,20><LT2><HX895>"));

  // Serial1.print(F("<F12><RC15,157><BS57,70>Entroponics<F9><RC25,785>TM"));

  // Serial1.print(F("<RC120,20><LT2><HX100><F11><RC95,0><CTR920>~Certificate of Completion~<RC120,815><LT2><HX100>"));

  // Serial1.print(F("<F3><RC150,95>Command was ")); //I spent
  // Serial1.print(digit); //,DEC

  // Serial1.print(F(" second"));
  // if(secsSpent!=1) Serial1.print(F("s"));

  // Serial1.print(F(" to save "));
  // Serial1.print(secsSaved,DEC);
  // Serial1.print(F(" second"));
  // if(secsSaved!=1) Serial1.print(F("s"));

  // Serial1.print(F("<F3><RC185,105>in the Entropon Deceleration Chamber<F9><RC185,825>TM"));

  // Serial1.print(F("<F9><RC228,0><CTR920>~at the Holistic Quantum Activation Art Expo~<F2><RC227,740>TM"));

  // Serial1.print(F("<F9><RC255,0><CTR920>~Vox Populi, Philadelphia, PA - September 20, 2024~"));

  // Serial1.print(F("<RC290,20><LT2><HX890><F11><RC305,0><CTR920>~"));
  // unsigned long mils = millis();
  // switch(mils % 4) {
  //   case 0: Serial1.print(F("Yesterday's Time...Today!")); break;
  //   case 1: Serial1.print(F("You're Not My Father, Time!")); break;
  //   case 2: Serial1.print(F("Retake Your Time!")); break;
  //   case 3: Serial1.print(F("It's Your Time to Unwind")); break;
  //   default: break;
  // }
  // Serial1.print(F("~"));

  // Serial1.print(F("<RC370,20><LT2><HX270><F9><RC360,320><BS20,15>entroponics.com<RC370,637><LT2><HX270>"));

  // Serial1.print(F("<RC390,980><RL><F3><CTR350>~Proof of~<RC390,1020><RL><F3><CTR350>~Entroponic~<RC390,1060><RL><F3><CTR350>~Deceleration~"));

  // Serial1.print(F("<p>"));
  // Serial1.flush();
}