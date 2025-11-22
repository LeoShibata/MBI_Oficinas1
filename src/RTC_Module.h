#ifndef RTC_MODULE_H
#define RTC_MODULE_H

#include <RTClib.h>
#include <Arduino.h>

/**
 * @brief Inicializa o DS3231 (usa Wire já inicializado no setup).
 * Por quê: manter simples e evitar múltiplos inits do I2C.
 */
void rtc_init();

/**
 * @brief Retorna o horário atual.
 * @return DateTime válido ou 2000-01-01 00:00:00 se RTC ausente.
 * Por quê: permite ao chamador identificar "no-rtc" sem travar.
 */
DateTime rtc_getTime();

#endif 