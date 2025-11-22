#include "RTC_Module.h"

static RTC_DS3231 rtc;
static bool s_ok = false;

void rtc_init() {
    s_ok = rtc.begin(); 
    if (!s_ok) {
        Serial.println("ERROR: RTC not found!");
        return;
    }
    if (rtc.lostPower()) {
        Serial.println("RTC without time. Setting time for compilation.");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
}

DateTime rtc_getTime() {
    if (!s_ok) return DateTime(2000, 1, 1, 0, 0, 0);
    return rtc.now();
}

DateTime now = rtc_getTime();
bool rtc_is_valid = !(now.year()==2000 && now.month()==1 && now.day()==1 &&
                      now.hour()==0 && now.minute()==0 && now.second()==0);

char rtc_buffer[32];
if (rtc_is_valid) {
    sprintf(rtc_buffer, "%02d/%02d/%04d %02d:%02d:%02d",
            now.day(), now.month(), now.year(),
            now.hour(), now.minute(), now.second());
} else {
    strcpy(rtc_buffer, "no-rtc");
}
if (ui_Label10) {
    lv_label_set_text(ui_Label10, rtc_buffer);
}

Serial.printf("LOG: User=%s, Sess=%s, Temp=%.2f C\n",
              currentUserId, currentSessionId, currentTemp);