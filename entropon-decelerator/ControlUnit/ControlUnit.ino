// ControlUnit.ino

#include "config.h"

#include "TimeTypes.h"
#include "DisplayManager.h"

#ifdef NETWORK_SSID
  #include <WiFiNINA.h>
#endif

#ifdef ENABLE_RTC
  #include <Wire.h>
  #include <RTClib.h>
  #include "RTCMillis.h"
#endif

// #include <SoftwareSerial.h>
#include "SerialProtocol.h"


// Global state
TimeValue normalTime;
TimeValue chamberTime;
ExhibitState state;
DisplayManager display;

//Used for capturing button presses from interrupts or serial
volatile bool btnStartPressed = false;
volatile bool btnStopPressed = false;

#ifdef ENABLE_RTC
  RTC_DS3231 rtc;
  RTCMillis* rtcMillis;
#endif

#ifdef NETWORK_SSID
  IPAddress printServer(BOCA_IP_A, BOCA_IP_B, BOCA_IP_C, BOCA_IP_D);
  WiFiClient lc; //A local client for sending TCP packets to server
#endif

SerialProtocol protocol(&Serial1);

// #ifdef PIN_PRINTER_TX
//   SoftwareSerial printerSerial(PIN_PRINTER_RX, PIN_PRINTER_TX);
// #endif

// Clock pulse tracking
bool normalClockOdd = false;
bool chamberClockOdd = false;
bool savedClockOdd = false;
uint8_t lastNormalSecond = 255;
uint8_t lastChamberSecond = 255;
uint32_t lastSavedSeconds = 0;

// Pots
uint16_t potMaxDecel = 512;
uint16_t potPower = 512;
uint16_t potRecovery = 512;

// Timing
uint32_t lastLoopMillis = 0;

void setup() {
  #ifdef ENABLE_SERIAL_LOGGING
    Serial.begin(115200);
  #endif
  #ifdef ENABLE_SERIAL_TO_CHAMBER_UNIT
    Serial1.begin(9600); //RX/TX serial to control unit
  #endif

  #ifdef PIN_PRINTER_TX
    printerSerial.begin(9600);
  #endif

  #ifdef NETWORK_SSID
    initNetwork();
  #endif
  
  // Initialize inputs
  #ifdef PIN_START_BUTTON
    pinMode(PIN_START_BUTTON,INPUT_PULLUP);
  #endif
  #ifdef PIN_STOP_BUTTON
    pinMode(PIN_STOP_BUTTON,INPUT_PULLUP);
  #endif
  #ifdef PIN_START_BUTTON_INT
    pinMode(PIN_START_BUTTON_INT,INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_START_BUTTON_INT), btnStartPress, FALLING);
  #endif
  #ifdef PIN_STOP_BUTTON_INT
    pinMode(PIN_STOP_BUTTON_INT,INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_STOP_BUTTON_INT), btnStopPress, FALLING);
  #endif

  #ifdef ENABLE_SERIAL_LOGGING
    //Delay welcome messages to here, to give time for serial to become available
    Serial.println(F("Hello from control unit."));
    #ifdef ENABLE_RTC
      Serial.println(F("Enter 'r' to set RTC after display test completes."));
    #endif
  #endif

  //The pot pins do not need init for analogRead()

  // Test displays
  display.begin();
  display.testPattern(WiFi.status()==WL_CONNECTED? 2: 1);

  #ifdef ENABLE_RTC
    if(!rtc.begin()) {
      Serial.println(F("RTC not found!"));
      //TODO update DisplayManager displayDiagnostics to accept a string to show this?
      while (1);
    }
    setRTC(); //See if we have input to set RTC by
    rtcMillis = new RTCMillis(&rtc);
    syncTimeFromRTC(); //Update local time constructs from RTC
    lastLoopMillis = rtcMillis->msm();
  #else
    lastLoopMillis = millis();
  #endif
      
  state.reset();
}

void loop() {

  #ifdef ENABLE_RTC
    uint32_t currentMillis = rtcMillis->msm();
  #else
    uint32_t currentMillis = millis();
  #endif

  uint32_t deltaMillis = currentMillis - lastLoopMillis;
  
  // Update times
  normalTime.addMillis(deltaMillis);
  
  // Calculate chamber time rate based on state
  updateChamberRate();
  
  // Calculate chamber time delta using the rate
  // This is a little imprecise over time, but so is Entroponics, really
  uint32_t chamberDelta = ((uint32_t)deltaMillis * state.chamberRateMs) / 1000;
  chamberTime.addMillis(chamberDelta);

  // State machine
  // Look for state signal from chamber unit
  #ifdef ENABLE_SERIAL_TO_CHAMBER_UNIT
    uint8_t msgType, payload[64];
    uint16_t payloadLength;
    if (protocol.receiveMessage(&msgType, payload, &payloadLength)) {
      //Debugging code removed after commit f60e5de
      if (msgType == SerialProtocol::MSG_STOP) {
        btnStopPressed = true; //as though a button interrupt was received
      }
    }
  #endif
  handleStateTransitions();
  
  // Send display data to chamber unit, so it can be updating displays at the same time as control unit
  #ifdef ENABLE_SERIAL_TO_CHAMBER_UNIT
    protocol.sendStateUpdate(state);
    protocol.sendTimeUpdate(normalTime, chamberTime);
  #endif

  // Update local displays
  display.updateLEDs(state.current,normalTime);
  display.updateRelays(state.current);
  display.updateMeter(state.chamberRateMs);
  display.updateNormalTime(normalTime);
  display.updateChamberTime(chamberTime,state.current);
  if(state.current == ExhibitState::DECELERATION) {
    display.updateSavedTime(normalTime,chamberTime);
    display.updateElapsedTime(state.elapsedMillis);
  }
  display.cycleAnalogClocks();
  
  // Re-sync with RTC at midnight - removed after commit f60e5de as it now happens continuously
  
  lastLoopMillis = currentMillis;
}

//Handle button interrupts
void btnStartPress() {
  btnStartPressed = true;
}
void btnStopPress() {
  btnStopPressed = true;
}

void updateChamberRate() {
    if(state.current==ExhibitState::NORMAL) return;

    //Read potentiometers
    //How slow can we let chamber time go?
    #ifdef PIN_POT_MAX_DECEL
      potMaxDecel = analogRead(PIN_POT_MAX_DECEL);
    #endif

    //How quickly do we get there?
    #ifdef PIN_POT_POWER
      potPower = analogRead(PIN_POT_POWER);
    #endif

    //How quickly do we recover?
    #ifdef PIN_POT_RECOVERY
      potRecovery = analogRead(PIN_POT_RECOVERY);
    #endif


    // Minimum rate from max decel pot: 0 = 1000ms/sec (time is normal), 1023 = 0ms/sec (time stands still)
    uint16_t minRate = (uint32_t)(1023-potMaxDecel) * 1000 / 1023;
    
    // Deceleration duration - min 2 seconds, max 300 seconds
    uint32_t decelDuration = 2000 + ((uint32_t)(1023 - potPower) * MAX_TIME-2000) / 1023;
    
    switch (state.current) {
      case ExhibitState::DECELERATION: {
        if (state.elapsedMillis >= decelDuration) {
            // Reached minimum rate
            state.chamberRateMs = minRate;
        } else {
            // Linear interpolation from 1000 to minRate
            uint32_t progress = (state.elapsedMillis * 1000) / decelDuration;
            state.chamberRateMs = 1000 - ((1000 - minRate) * progress) / 1000;
        }
        break;
      }
      
      case ExhibitState::RECOVERY: {
        //Go as fast as the analog clock will tick
        state.chamberRateMs = map(potRecovery, 0, 1023, 0, 10000);
      }
    }
}

void handleStateTransitions() {
    static uint32_t decelStartTime = 0;
    static uint32_t recoverStartTime = 0;
    
    switch (state.current) {
        case ExhibitState::NORMAL:
            if ( btnStartPressed
              #ifdef PIN_START_BUTTON
                || digitalRead(PIN_START_BUTTON) == PIN_START_BUTTON_PRESSED
              #endif
            ) {
                state.current = ExhibitState::DECELERATION;
                #ifdef ENABLE_SERIAL_LOGGING
                  Serial.println("Decelerating");
                #endif
                state.elapsedMillis = 0;
                state.chamberRateMs = 500;
                #ifdef ENABLE_RTC
                  decelStartTime = rtcMillis->msm();
                #else
                  decelStartTime = millis();
                #endif
            }
            break;
            
        case ExhibitState::DECELERATION:
            #ifdef ENABLE_RTC
              state.elapsedMillis = rtcMillis->msm() - decelStartTime;
            #else
              state.elapsedMillis = millis() - decelStartTime;
            #endif
            
            if ( btnStopPressed
              #ifdef PIN_STOP_BUTTON
                || digitalRead(PIN_STOP_BUTTON) == PIN_STOP_BUTTON_PRESSED
              #endif
              || state.chamberRateMs <= 100 // Within 3% of minimum
            ) {  
                printCertificate(); //needs state.elapsedMillis to not be zeroed yet
                state.current = ExhibitState::RECOVERY;
                #ifdef ENABLE_SERIAL_LOGGING
                  Serial.println("Recovering");
                #endif
                state.elapsedMillis = 0;
                state.chamberRateMs = 2400;
                #ifdef ENABLE_RTC
                  recoverStartTime = rtcMillis->msm();
                #else
                  recoverStartTime = millis();
                #endif
            }
            break;
            
        case ExhibitState::RECOVERY:
            #ifdef ENABLE_RTC
              state.elapsedMillis = rtcMillis->msm() - recoverStartTime;
            #else
              state.elapsedMillis = millis() - recoverStartTime;
            #endif
            
            if (normalTime.getDifferenceMillis(chamberTime)<0) {
            // if (state.chamberRateMs >= 1000-(1000/100)) { //within 1% of 1000
                state.chamberRateMs = 1000;
                state.current = ExhibitState::NORMAL;
                #ifdef ENABLE_SERIAL_LOGGING
                  Serial.println("Normal");
                #endif
                state.elapsedMillis = 0;
                // Resync times
                chamberTime = normalTime;
            }
            break;
    } //end switch

    //Discard button presses, whether we acted on them or not (per current state)
    btnStartPressed = false;
    btnStopPressed = false;
}

void setRTC() {
  #ifdef ENABLE_RTC
    #ifdef ENABLE_SERIAL_LOGGING
      //Check for serial console input to set RTC
      if(Serial.available()) {
        char incomingChar = Serial.read();
        while(Serial.available()) Serial.read(); //dump the rest
        if(incomingChar != 'r') {
          Serial.print("Unknown command: ");
          Serial.println(incomingChar);
          return;
        }
      } else {
        Serial.println("No command received.");
        return;
      }
      //setting!
      int hr = 0;
      Serial.println("Setting RTC. Enter hour:");
      while(1) {
        if(Serial.available()) {
          hr = Serial.parseInt()%24;
          while(Serial.available()) Serial.read();
          break;
        }
      }
      int min = 0;
      Serial.println("Enter minute, at top of minute:");
      while(1) {
        if(Serial.available()) {
          min = Serial.parseInt()%60;
          while(Serial.available()) Serial.read();
          break;
        }
      }
      rtc.adjust(DateTime(2025, 1, 1, hr, min, 0));
    #endif
  #endif
}

void syncTimeFromRTC() {
  #ifdef ENABLE_RTC
    DateTime tod = rtc.now();
    normalTime.setTime(tod.hour(),tod.minute(),tod.second());
    chamberTime.setTime(tod.hour(),tod.minute(),tod.second());
    //TODO compare time to new time, so you know to advance analog clocks
    // uint32_t msm = rtcMillis->msm();
    // normalTime.setTimeMSM(msm);
    // chamberTime.setTimeMSM(msm);
  #endif
}

void initNetwork(){
  //Skipping checking status of wifi module
  networkStartWiFi();
}

void networkStartWiFi(){
  #ifdef NETWORK_SSID
    #ifdef ENABLE_SERIAL_LOGGING
      Serial.print(F("Attempting to connect to SSID: ")); Serial.println(NETWORK_SSID);
    #endif

    WiFi.begin(NETWORK_SSID, NETWORK_PASS); //WPA - hangs while connecting
    if(WiFi.status()==WL_CONNECTED){ //did it work?
      #ifdef ENABLE_SERIAL_LOGGING
        Serial.println(F("Connected!"));
        Serial.print(F("SSID: ")); Serial.println(WiFi.SSID());
        Serial.print(F("Signal strength (RSSI):")); Serial.print(WiFi.RSSI()); Serial.println(F(" dBm"));
      #endif
    }
    else {
      #ifdef ENABLE_SERIAL_LOGGING
        Serial.println(F("Wasn't able to connect."));
      #endif
    }
  #endif
} //end fn startWiFi

void networkDisconnectWiFi(){
  #ifdef NETWORK_SSID
    #ifdef ENABLE_SERIAL_LOGGING
      Serial.println(F("Disconnecting WiFi"));
    #endif
    WiFi.end();
  #endif
}

void printCertificate(){
  uint32_t timeDiff = abs((int32_t)(normalTime.millisSinceMidnight - 
                            chamberTime.millisSinceMidnight));
  uint32_t secsSaved = timeDiff / 1000;
  uint32_t secsSpent = state.elapsedMillis / 1000;

  #ifdef ENABLE_SERIAL_LOGGING
    Serial.print(F("Printing certificate for "));
    Serial.print(secsSpent,DEC);
    Serial.print(F("sec spent, "));
    Serial.print(secsSaved,DEC);
    Serial.println(F("sec saved"));
  #endif
  
  #ifdef NETWORK_TRY_PRINT
  if(WiFi.status()!=WL_CONNECTED) {
    #ifdef ENABLE_SERIAL_LOGGING
      Serial.println(F("Not connected to WiFi; print attempt aborted"));
    #endif
    return;
  }
  
  //https://stackoverflow.com/a/74554673
  delay(50);
  if (lc.connect(printServer, BOCA_IP_PORT)) {
    if (lc.connected()) {
      #ifdef ENABLE_SERIAL_LOGGING
        Serial.println(F("Printing now"));
      #endif
      


      lc.print(F("<RC11,15><LT2><HX900><TTF1,24><RC13,52.5><CTR75>~E~<RC13,127.5><CTR75>~N~<RC13,202.5><CTR75>~T~<RC13,277.5><CTR75>~R~<RC13,352.5><CTR75>~O~<RC13,427.5><CTR75>~P~<RC13,502.5><CTR75>~O~<RC13,577.5><CTR75>~N~<RC13,652.5><CTR75>~I~<RC13,727.5><CTR75>~C~<RC13,802.5><CTR75>~S~<RC24,860><TTF1,7>TM<RC113,15><LT2><HX110><RC92,5><F11><CTR900>~Certificate of Completion~<RC113,805><LT2><HX110><RC140,15><TTF1,13><CTR900>~I spent "));

      lc.print(secsSpent,DEC);
      lc.print(F(" second"));
      if(secsSpent!=1) lc.print(F("s"));
      lc.print(F(" to save "));
      lc.print(secsSaved,DEC);
      lc.print(F(" second"));
      if(secsSaved!=1) lc.print(F("s"));

      lc.print(F("~<RC178,15><TTF1,13><CTR900>~in the Entropon Decelerator~<TTF1,7><RC177,680>TM<RC223,15><TTF1,10><CTR900>~at the Holistic Quantum Activation Art Expo~<RC255,15><TTF1,10><CTR900>~Philadelphia, PA - November 14, 2025~<RC302,15><LT2><HX900><RC316,5><F11><CTR900>~"));

      unsigned long mils = millis(); //as a source of randomness, this is fine
      switch(mils % 5) {
        case 0: lc.print(F("Yesterday's Time...Today!")); break;
        case 1: lc.print(F("You're Not My Father, Time!")); break;
        case 2: lc.print(F("Retake Your Time!")); break;
        case 3: lc.print(F("It's Your Time to Unwind")); break;
        case 4: lc.print(F("Your Moment is Now")); break;
        default: break;
      }

      lc.print(F("~<RC379,15><LT2><HX350><RC360,15><TTF1,9><CTR900>~entroponics.com~<RC379,565><LT2><HX350><RC390,967><RL><TTF1,13><CTR390>~Proof of~<RC390,1005><RL><TTF1,13><CTR390>~Entroponic~<RC390,1043><RL><TTF1,13><CTR390>~Deceleration~<p>"));


      // lc.print(F("<RC10,20><LT2><HX895>"));

      // lc.print(F("<F12><RC15,157><BS57,70>Entroponics<F9><RC25,785>TM"));

      // lc.print(F("<RC120,20><LT2><HX100><F11><RC95,0><CTR920>~Certificate of Completion~<RC120,815><LT2><HX100>"));

      // lc.print(F("<F3><RC150,95>I spent "));
      // lc.print(secsSpent,DEC);
      // lc.print(F(" second"));
      // if(secsSpent!=1) lc.print(F("s"));

      // lc.print(F(" to save "));
      // lc.print(secsSaved,DEC);
      // lc.print(F(" second"));
      // if(secsSaved!=1) lc.print(F("s"));

      // lc.print(F("<F3><RC185,105>in the Entropon Deceleration Chamber<F9><RC185,825>TM"));

      // lc.print(F("<F9><RC228,0><CTR920>~at the Holistic Quantum Activation Art Expo~")); //<F2><RC227,740>TM

      // lc.print(F("<F9><RC255,0><CTR920>~Philadelphia, PA - November 14, 2025~"));

      // lc.print(F("<RC290,20><LT2><HX890><F11><RC305,0><CTR920>~"));
      // unsigned long mils = millis();
      // switch(mils % 4) {
      //   case 0: lc.print(F("Yesterday's Time...Today!")); break;
      //   case 1: lc.print(F("You're Not My Father, Time!")); break;
      //   case 2: lc.print(F("Retake Your Time!")); break;
      //   case 3: lc.print(F("It's Your Time to Unwind")); break;
      //   default: break;
      // }
      // lc.print(F("~"));

      // lc.print(F("<RC370,20><LT2><HX270><F9><RC360,320><BS20,15>entroponics.com<RC370,637><LT2><HX270>"));

      // lc.print(F("<RC390,980><RL><F3><CTR350>~Proof of~<RC390,1020><RL><F3><CTR350>~Entroponic~<RC390,1060><RL><F3><CTR350>~Deceleration~"));

      // lc.print(F("<p>"));


      lc.flush();
      lc.stop();
    } else {
      #ifdef ENABLE_SERIAL_LOGGING
        Serial.println(F("Could not connect to printer"));
      #endif
    }
    // while (!lc.available());                // wait for response
    // String str = lc.readStringUntil('\n');  // read entire response
    // Serial.print("[Rx] ");
    // Serial.println(str);
    // lc.disconnect(); //whaaaaa
  } else {
    #ifdef ENABLE_SERIAL_LOGGING
      Serial.println(F("Could not create connection to printer"));
    #endif
  }
  #else
    #ifdef ENABLE_SERIAL_LOGGING
      Serial.println(F("Not configured to print; print attempt aborted"));
    #endif
  #endif
}