#ifndef RTC_H
#define RTC_H

#include <RTClib.h>

void rtc_init();        // Inicializa o RTC
DateTime rtc_getTime(); // Retorna a hora atual

#endif
