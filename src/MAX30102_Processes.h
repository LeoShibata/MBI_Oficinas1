#pragma once
#include <stdint.h>

/** @brief Inicializa processador PPG (ex.: 100 sps). */
void ppg_init(int sample_rate_sps);

/** @brief Zera estados internos (DC/AC/ buffers / BPM / SpO2). */
void ppg_reset();

/** @brief Define DC mínimo no IR para considerar dedo presente. */
void ppg_setFingerThreshold(uint32_t ir_dc_min);

/** @brief Alimenta o processador com 1 amostra RED/IR. */
void ppg_feedSample(uint32_t red, uint32_t ir);

/** @brief Tick de 1s para consolidar RMS/R/SpO2 e validar BPM. */
void ppg_tick_1s();

/** @brief Sinaliza se há SpO2 válido. */  bool  ppg_hasSpO2();
/** @brief Retorna SpO2. */                float ppg_getSpO2();
/** @brief Sinaliza se há BPM válido. */   bool  ppg_hasBPM();
/** @brief Retorna BPM. */                 float ppg_getBPM();
