#include <arduino.h>
#include "entropon-decelerator.h"

#include "networkNINA.h"
#include <WiFiNINA.h>
#include <WiFiUdp.h>
//Needs to be able to control the RTC
// #include "rtcDS3231.h"
// #include "rtcMillis.h"


unsigned int localPort = 2390; // local port to listen for UDP packets
IPAddress ntpServer(129, 6, 15, 28); // time.nist.gov NTP server
#define NTP_PACKET_SIZE 48 // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP Udp; // A UDP instance to let us send and receive packets over UDP
#define NTP_TIMEOUT 1000 //how long to wait for a request to finish - the longer it takes, the less reliable the result is
#define NTP_MINFREQ 5000 //how long to enforce a wait between request starts (NIST requires at least 4sec between requests or will ban the client)
const unsigned long NTPOK_THRESHOLD = 3600000; //if no sync within 60 minutes, the time is considered stale


IPAddress printServer(BOCA_IP_A, BOCA_IP_B, BOCA_IP_C, BOCA_IP_D);
WiFiClient lc; //A local client for sending TCP packets to server



void initNetwork(){
  //Skipping checking status of wifi module
  networkStartWiFi();
}
void cycleNetwork(){
  #ifdef NETWORK_TRY_NTP
  checkNTP(); //to see what's come back
  #endif
}

void networkStartWiFi(){
  Serial.print(F(" Attempting to connect to SSID: ")); Serial.println(NETWORK_SSID);

  // WiFi.begin(NETWORK_SSID.c_str(), NETWORK_PASS.c_str()); //WPA - hangs while connecting
  WiFi.begin(NETWORK_SSID, NETWORK_PASS); //WPA - hangs while connecting
  if(WiFi.status()==WL_CONNECTED){ //did it work?
  
    Serial.print(millis()); Serial.println(F(" Connected!"));
    Serial.print(F("SSID: ")); Serial.println(WiFi.SSID());
    Serial.print(F("Signal strength (RSSI):")); Serial.print(WiFi.RSSI()); Serial.println(F(" dBm"));

    #ifdef NETWORK_TRY_NTP
    Udp.begin(localPort); cueNTP();
    #endif
  }
  else Serial.println(F(" Wasn't able to connect."));
} //end fn startWiFi

void networkDisconnectWiFi(){
  Serial.println(F("Disconnecting WiFi"));
  WiFi.end();
}

bool ntpCued = false;
unsigned long ntpStartLast = 0; //zero is a special value meaning it has never been used
bool ntpGoing = 0;
unsigned long ntpSyncLast = 0; //zero is a special value meaning it has never been used
unsigned long ntpTime = 0; //When this is nonzero, it means we have captured a time and are waiting to set the clock until the next full second, in order to achieve subsecond setting precision (or close to - it'll be behind by up to the loop time, since we aren't simply using delay() in order to keep the nixie display going). TODO account for future epochs which could result in a valid 0 value
unsigned  int ntpMils = 0;

unsigned long ntpSyncAgo(){
  if(!ntpSyncLast || ntpTime) return 86400000; //if we haven't synced before, or are waiting for a set to apply TODO epoch issue
  // In cases where NTP fails chronically (e.g. wifi disconnect, bad server, etc), we don't want to risk this rolling over after 49 days and professing to be correct. So each time we check this, if the diff is greater than our "NTP OK" range (24 hours), we'll bump up ntpSyncLast so it only just fails to qualify.
  unsigned long now = millis();
  if((unsigned long)(now-ntpSyncLast)>86400000){
    ntpSyncLast = (unsigned long)(now-86400000);
    if(!ntpSyncLast) ntpSyncLast = -1; //never let it be zero
  }
  return (unsigned long)(now-ntpSyncLast);
}

void cueNTP(){
  // We don't want to let other code startNTP() directly since it's normally asynchronous, and that other code may delay the time until we can check the result. Exception is forced call from admin page, which calls startNTP() synchronously.
  #ifdef NETWORK_TRY_NTP
  ntpCued = true;
  #endif
}

int startNTP(bool synchronous){ //Called at intervals to check for ntp time
  #ifdef NETWORK_TRY_NTP
  //synchronous is for forced call from admin page, so we can return an error code, or 0 on successful sync
  // if(wssid==F("")) return -1; //don't try to connect if there's no creds
  if(WiFi.status()!=WL_CONNECTED && WiFi.status()!=WL_AP_CONNECTED && WiFi.status()!=WL_AP_LISTENING) networkStartWiFi(); //in case the wifi dropped. Don't try if currently offering an access point.
  if(WiFi.status()!=WL_CONNECTED) return -2;
  if(ntpGoing || ntpTime) return -3; //if request going, or waiting to set to apply TODO epoch issue
  if((unsigned long)(millis()-ntpStartLast) < NTP_MINFREQ) return -4; //if a previous request is going, do not start another until at least NTP_MINFREQ later
  Serial.print(millis(),DEC); Serial.println(F(" NTP starting"));
  ntpGoing = 1;
  ntpStartLast = millis(); if(!ntpStartLast) ntpStartLast = -1; //never let it be zero
  Udp.flush(); //in case of old data
  //Udp.stop() was formerly here
  Serial.println(); Serial.print(millis()); Serial.println(F(" Sending UDP packet to NTP server."));
  memset(packetBuffer, 0, NTP_PACKET_SIZE); // set all bytes in the buffer to 0
  // Initialize values needed to form NTP request
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  Udp.beginPacket(ntpServer, 123);
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
  if(synchronous){
    bool success = false;
    while(!success && (unsigned long)(millis()-ntpStartLast)<NTP_TIMEOUT){
      success = checkNTP(); //will return true when we successfully got a time to sync to
    }
    return (success? 0: -5);
  }
  checkNTP(); //asynchronous - may as well go ahead and check in case it comes back quickly enough
  #endif
} //end fn startNTP

bool checkNTP(){ //Called on every cycle to see if there is an ntp response to handle
  #ifdef NETWORK_TRY_NTP
  //Return whether we had a successful sync - used for forced call from admin page, via synchronous startNTP()
  if(ntpGoing){
    //If we are waiting for a packet that hasn't arrived, wait for the next cycle, or time out
    if(!Udp.parsePacket()){
      if((unsigned long)(millis()-ntpStartLast)>=NTP_TIMEOUT) ntpGoing = 0; //time out
      return false;
    }
    // We've received a packet, read the data from it
    ntpSyncLast = millis(); if(!ntpSyncLast) ntpSyncLast = -1; //never let it be zero
    unsigned int requestTime = ntpSyncLast-ntpStartLast;
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
  
    //https://forum.arduino.cc/index.php?topic=526792.0
    //epoch in earlier bits? needed after 2038
    //TODO leap second notification in earlier bits?
    ntpTime = (packetBuffer[40] << 24) | (packetBuffer[41] << 16) | (packetBuffer[42] << 8) | packetBuffer[43];
    unsigned long ntpFrac = (packetBuffer[44] << 24) | (packetBuffer[45] << 16) | (packetBuffer[46] << 8) | packetBuffer[47];
    ntpMils = (int32_t)(((float)ntpFrac / UINT32_MAX) * 1000);
    
    //Account for the request time
    ntpMils += requestTime/2;
    if(ntpMils>=1000) { ntpMils -= 1000; ntpTime++; }
    
    Serial.print(F("NTP time: "));
    Serial.print(ntpTime,DEC);
    Serial.print(F("."));
    Serial.print(ntpMils,DEC);
    Serial.print(F(" ±"));
    Serial.print(requestTime,DEC);
  
    //Unless the mils are bang on, we'll wait to set the clock until the next full second.
    if(ntpMils>0) ntpTime++;
    
    Serial.print(F(" - set to "));
    Serial.print(ntpTime,DEC);
    if(ntpMils==0) Serial.print(F(" immediately"));
    else { Serial.print(F(" after ")); Serial.print(1000-ntpMils,DEC); }
    Serial.println();

    Udp.flush(); //in case of extraneous(?) data
    //Udp.stop() was formerly here
    ntpGoing = 0; //next if{} block will handle this
  }
  if(!ntpGoing){
    //If we are waiting to start, do it (asynchronously)
    if(ntpCued){ startNTP(false); ntpCued=false; return false; }
    //If we are not waiting to set, do nothing
    if(!ntpTime) return false;
    //If we are waiting to set, but it's not time, wait for the next cycle
    //but return true since we successfully got a time to set to
    if(ntpMils!=0 && (unsigned long)(millis()-ntpSyncLast)<(1000-ntpMils)) return true;
    //else it's time!
    Serial.print(millis(),DEC); Serial.println(F("NTP complete"));

    //Convert unix timestamp to UTC date/time
    //TODO this assumes epoch 0, which is only good til 2038, I think!
    ntpTime -= 3155673600; //from 1900 to 2000, assuming epoch 0
    unsigned long ntpPart = ntpTime;
    int y = 2000;
    while(1){ //iterate to find year
      unsigned long yearSecs = daysInYear(y)*86400;
      if(ntpPart > yearSecs){
        ntpPart-=yearSecs; y++;
      } else break;
    }
    byte m = 1;
    while(1){ //iterate to find month
      unsigned long monthSecs = daysInMonth(y,m)*86400;
      if(ntpPart > monthSecs){
        ntpPart-=monthSecs; m++;
      } else break;
    }
    byte d = 1+(ntpPart/86400); ntpPart %= 86400;
    int hm = ntpPart/60; //mins from midnight
    byte s = ntpPart%60;
  
    //Take UTC date/time and apply standard offset
    //which involves checking for date rollover
    //eeprom loc 14 is UTC offset in quarter-hours plus 100 - range is 52 (-12h or -48qh, US Minor Outlying Islands) to 156 (+14h or +56qh, Kiribati)
    int utcohm = (readEEPROM(14,false)-100)*15; //utc offset in mins from midnight
    if(hm+utcohm<0){ //date rolls backward
      hm = hm+utcohm+1440; //e.g. -1 to 1439 which is 23:59
      d--; if(d<1){ m--; if(m<1){ y--; m=12; } d=daysInMonth(y,m); } //month or year rolls backward
    } else if(hm+utcohm>1439){ //date rolls forward
      hm = (hm+utcohm)%1440; //e.g. 1441 to 1 which is 00:01
      d++; if(d>daysInMonth(y,m)){ m++; if(m>12){ y++; m=1; } d=1; } //month or year rolls forward
    } else hm += utcohm;
  
    //then check DST at that time (setting DST flag), and add an hour if necessary
    //which involves checking for date rollover again (forward only)
    //TODO this may behave unpredictably from 1–2am on fallback day since that occurs twice - check to see whether it has been applied already per the difference from utc
    if(isDSTByHour(y,m,d,hm/60,true)){
      if(hm+60>1439){ //date rolls forward
        hm = (hm+60)%1440; //e.g. 1441 to 1 which is 00:01
        d++; if(d>daysInMonth(y,m)){ m++; if(m>12){ y++; m=1; } d=1; } //month or year rolls forward
      } else hm += 60;
    }
  
    //finally set the rtc
    rtcSetDate(y, m, d, dayOfWeek(y,m,d));
    rtcSetTime(hm/60,hm%60,s);
    calcSun();
  
    Serial.print(F("RTC set to "));
    Serial.print(rtcGetYear(),DEC); Serial.print(F("-"));
    if(rtcGetMonth()<10) Serial.print(F("0")); Serial.print(rtcGetMonth(),DEC); Serial.print(F("-"));
    if(rtcGetDate()<10) Serial.print(F("0")); Serial.print(rtcGetDate(),DEC); Serial.print(F(" "));
    if(rtcGetHour()<10) Serial.print(F("0")); Serial.print(rtcGetHour(),DEC); Serial.print(F(":"));
    if(rtcGetMinute()<10) Serial.print(F("0")); Serial.print(rtcGetMinute(),DEC); Serial.print(F(":"));
    if(rtcGetSecond()<10) Serial.print(F("0")); Serial.print(rtcGetSecond(),DEC);
    Serial.println();
    
    ntpTime = 0; ntpMils = 0; //no longer waiting to set
    updateDisplay();
    return true; //successfully got a time and set to it
  }
  #endif
} //end fn checkNTP

void clearNTPSyncLast(){
  #ifdef NETWORK_TRY_NTP
  //called when other code divorces displayed time from NTP sync
  ntpSyncLast = 0;
  #endif
}

void printCertificate(int secsSpent, int secsSaved){
  #ifdef NETWORK_TRY_PRINT
  //https://stackoverflow.com/a/74554673
  if (lc.connect(printServer, BOCA_IP_PORT)) {
    if (lc.connected()) {
      lc.print(F("<RC10,20><LT2><HX830>"));

      lc.print(F("<F12><RC15,122><BS57,70>Entroponics<F9><RC25,730>TM"));

      lc.print(F("<RC120,20><LT2><HX80><F11><RC95,0><CTR850>~Certificate of Completion~<RC120,770><LT2><HX80>"));

      lc.print(F("<F3><RC150,60>I spent "));
      lc.print(secsSpent,DEC);
      lc.print(F(" second"));
      if(secsSpent!=1) lc.print(F("s"));

      lc.print(F(" to save "));
      lc.print(secsSaved,DEC);
      lc.print(F(" second"));
      if(secsSaved!=1) lc.print(F("s"));

      lc.print(F("<F3><RC185,70>in the Entropon Deceleration Chamber<F9><RC185,790>TM"));

      lc.print(F("<F9><RC228,0><CTR850>~at the Holistic Quantum Activation Art Expo~<F2><RC227,705>TM"));

      lc.print(F("<F9><RC255,0><CTR850>~Vox Populi, Philadelphia, PA - September 20, 2024~"));

      lc.print(F("<RC290,20><LT2><HX830><F11><RC305,0><CTR850>~"));
      unsigned long mils = millis();
      switch(mils % 4) {
        case 0: lc.print(F("Yesterday's Time...Today!")); break;
        case 1: lc.print(F("You're Not My Father, Time!")); break;
        case 2: lc.print(F("Retake Your Time!")); break;
        case 3: lc.print(F("It's Your Time to Unwind")); break;
      }
      lc.print(F("~"));

      lc.print(F("<RC370,20><LT2><HX250><F9><RC360,280><BS20,15>entroponics.com<RC370,600><LT2><HX250>"));

      lc.print(F("<RC50,900><F3>I Survived<RC85,900><F3>Entroponic<RC120,900><F3><BS16,20>Deceleration<RC155,900><F3><BS15,20>And All I Got<RC190,900><F3><BS26,20>Was This<RC230,900><F11><BS43,20>Lousy<RC280,900><F11><BS24,20>Hair Net"));

      lc.print(F("<p>"));
    }
    // while (!lc.available());                // wait for response
    // String str = lc.readStringUntil('\n');  // read entire response
    // Serial.print("[Rx] ");
    // Serial.println(str);
    // lc.disconnect(); //whaaaaa
  }
  #endif
}

//cf. arduino-clock for admin stuff