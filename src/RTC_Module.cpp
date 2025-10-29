#include "RTC_Module.h"

RTC_DS3231 rtc;

void rtc_init() {
    if(!rtc.begin()) {
        Serial.println("ERROR: RTC not found!");
        return;
    } 
    else {                                                                                                                    
        if(rtc.lostPower()) {
            Serial.println("RTC without time. Setting time for compilation.");
            rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        }
    }
}

DateTime rtc_getTime() {
    return rtc.now();
}
