#ifndef RTC_MILLIS_H
#define RTC_MILLIS_H

#include <Arduino.h>
#include <RTClib.h>

class RTCMillis {
private:
    RTC_DS3231* rtc;

    uint32_t lastSSM; //secs since midnight per RTC
    uint32_t millisAtLastSSM;
    bool doneALog = false;
    
public:
    RTCMillis(RTC_DS3231* rtcRef) : rtc(rtcRef) {}

    uint32_t msm() {
        //Returns millis since midnight per RTC plus a bit of real millis()
        DateTime tod = rtc->now();
        uint32_t curSSM = tod.hour() * 3600UL + tod.minute() * 60UL + tod.second();
        if(curSSM!=lastSSM) {
            millisAtLastSSM = millis();
            lastSSM = curSSM;
        }
        return curSSM * 1000UL + (millis()-millisAtLastSSM);
        // return millis();
    }
};

#endif // RTC_MILLIS_H