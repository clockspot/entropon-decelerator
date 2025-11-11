// SerialTest.ino

#include "config.h"

void setup() {
  delay(2000);
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Hello world");
}

void loop() {
  int readVal = 0;
  #ifdef PIN_POT_DECEL
    readVal = analogRead(PIN_POT_DECEL);
    Serial.print(readVal); Serial.print(" ");
  #endif
  #ifdef PIN_POT_POWER
    readVal = analogRead(PIN_POT_POWER);
    Serial.print(readVal); Serial.print(" ");
  #endif
  #ifdef PIN_POT_RECOVERY
    readVal = analogRead(PIN_POT_RECOVERY);
    Serial.print(readVal); Serial.print(" ");
  #endif
  Serial.println();
  delay(2000);
}