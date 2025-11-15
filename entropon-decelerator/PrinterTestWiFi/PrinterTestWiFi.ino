#include "config.h"
#include <WiFiNINA.h>

void setup() {
  delay(2000);
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Hello world");

  initNetwork();
}

void loop() {
  if(Serial.available()) {
    char incomingChar = Serial.read();
    if(incomingChar >= '0' && incomingChar <= '9') {
      int digit = incomingChar - '0';
      Serial.print("Command: ");
      Serial.println(digit);
      printCertificate(digit,digit);
    } else {
      Serial.println("Unrecognized command");
    }
    while(Serial.available()) Serial.read();
  }
}

IPAddress printServer(BOCA_IP_A, BOCA_IP_B, BOCA_IP_C, BOCA_IP_D);
WiFiClient lc; //A local client for sending TCP packets to server

void initNetwork(){
  //Skipping checking status of wifi module
  networkStartWiFi();
}

void networkStartWiFi(){
  #ifdef NETWORK_SSID
  Serial.print(F(" Attempting to connect to SSID: ")); Serial.println(NETWORK_SSID);

  // WiFi.begin(NETWORK_SSID.c_str(), NETWORK_PASS.c_str()); //WPA - hangs while connecting
  WiFi.begin(NETWORK_SSID, NETWORK_PASS); //WPA - hangs while connecting
  if(WiFi.status()==WL_CONNECTED){ //did it work?
  
    Serial.print(millis()); Serial.println(F(" Connected!"));
    Serial.print(F("SSID: ")); Serial.println(WiFi.SSID());
    Serial.print(F("Signal strength (RSSI):")); Serial.print(WiFi.RSSI()); Serial.println(F(" dBm"));
  }
  else Serial.println(F(" Wasn't able to connect."));
  #endif
} //end fn startWiFi

void networkDisconnectWiFi(){
  #ifdef NETWORK_SSID
  Serial.println(F("Disconnecting WiFi"));
  WiFi.end();
  #endif
}

void printCertificate(int secsSpent, int secsSaved){
  Serial.print(F("Printing "));
  Serial.print(secsSpent,DEC);
  Serial.print(F("/"));
  Serial.print(secsSaved,DEC);
  Serial.println();
  #ifdef NETWORK_TRY_PRINT
  //https://stackoverflow.com/a/74554673
  delay(50);
  if (lc.connect(printServer, BOCA_IP_PORT)) {
    if (lc.connected()) {
      Serial.println(F("Printing now")); //ser ial
      lc.print(F("<RC10,20><LT2><HX895>"));

      lc.print(F("<F12><RC15,157><BS57,70>Entroponics<F9><RC25,785>TM"));

      lc.print(F("<RC120,20><LT2><HX100><F11><RC95,0><CTR920>~Certificate of Completion~<RC120,815><LT2><HX100>"));

      lc.print(F("<F3><RC150,95>I spent "));
      lc.print(secsSpent,DEC);
      lc.print(F(" second"));
      if(secsSpent!=1) lc.print(F("s"));

      lc.print(F(" to save "));
      lc.print(secsSaved,DEC);
      lc.print(F(" second"));
      if(secsSaved!=1) lc.print(F("s"));

      lc.print(F("<F3><RC185,105>in the Entropon Deceleration Chamber<F9><RC185,825>TM"));

      lc.print(F("<F9><RC228,0><CTR920>~at the Holistic Quantum Activation Art Expo~")); //<F2><RC227,740>TM

      lc.print(F("<F9><RC255,0><CTR920>~Philadelphia, PA - November 14, 2025~"));

      lc.print(F("<RC290,20><LT2><HX890><F11><RC305,0><CTR920>~"));
      unsigned long mils = millis();
      switch(mils % 4) {
        case 0: lc.print(F("Yesterday's Time...Today!")); break;
        case 1: lc.print(F("You're Not My Father, Time!")); break;
        case 2: lc.print(F("Retake Your Time!")); break;
        case 3: lc.print(F("It's Your Time to Unwind")); break;
        default: break;
      }
      lc.print(F("~"));

      lc.print(F("<RC370,20><LT2><HX270><F9><RC360,320><BS20,15>entroponics.com<RC370,637><LT2><HX270>"));

      lc.print(F("<RC390,980><RL><F3><CTR350>~Proof of~<RC390,1020><RL><F3><CTR350>~Entroponic~<RC390,1060><RL><F3><CTR350>~Deceleration~"));

      lc.print(F("<p>"));
      lc.flush();
      lc.stop();
    }
    // while (!lc.available());                // wait for response
    // String str = lc.readStringUntil('\n');  // read entire response
    // Serial.print("[Rx] ");
    // Serial.println(str);
    // lc.disconnect(); //whaaaaa
  }
  #endif
}