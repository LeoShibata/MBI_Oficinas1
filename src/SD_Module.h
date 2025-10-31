#pragma once
#include <Arduino.h>

void CSV_begin();
bool openDailyIfNeeded();
bool CSV_rotateIfNeeded();  // stub no MVP
bool CSV_appendRow(const char* userId,
                   const char* sessionId,
                   const char* sensor,
                   double valor,
                   const char* unidade);
void CSV_close();
