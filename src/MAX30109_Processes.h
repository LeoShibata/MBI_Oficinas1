#pragma once
#include <stdint.h>

void ppg_init(int sample_rate_sps);
void ppg_reset();
void ppg_setFingerThreshold(uint32_t ir_dc_min);

void ppg_feedSample(uint32_t red, uint32_t ir);
void ppg_tick_1s();

bool  ppg_hasSpO2();
float ppg_getSpO2();

bool  ppg_hasBPM();
float ppg_getBPM();
