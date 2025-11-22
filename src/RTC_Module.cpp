#include "RTC_Module.h"

static RTC_DS3231 rtc;
static bool s_ok = false;

void rtc_init() {
  s_ok = rtc.begin(); // por quê: apenas grava status; sem complexidade extra
  if (!s_ok) {
    Serial.println("ERROR: RTC not found!");
    return;
  }
  if (rtc.lostPower()) {
    Serial.println("RTC without time. Setting compile time.");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

DateTime rtc_getTime() {
  // por quê: evita chamar now() quando begin() falhou
  if (!s_ok) return DateTime(2000, 1, 1, 0, 0, 0);
  return rtc.now();
}
