#pragma once
#include <stdint.h>

bool max30102_begin();
bool max30102_start();
bool max30102_stop();

bool max30102_available();
bool max30102_readRaw(uint32_t* red, uint32_t* ir);

bool max30102_isPresent();

