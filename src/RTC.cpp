#include "RTC.h"
#include <Arduino.h>

RTC_DS3231 rtc;

void rtc_init() {
    if (!rtc.begin()) {
        Serial.println("Erro: RTC não encontrado!");
    } else {
        if (rtc.lostPower()) {
            Serial.println("RTC sem hora. Ajustando hora para compilação.");
            rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        }
    }
}

DateTime rtc_getTime() {
    return rtc.now();
}
