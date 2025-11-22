#pragma once

/**
 * @brief Inicializa o AD8232 (via ADC do ESP32).
 * Por quê: configura pino, atenuação (11 dB) e 12 bits.
 */
void ad8232_init();

/**
 * @brief Lê ECG em mV com baseline removida (centrado em 0).
 * @return mV relativos (±), -999.f se erro/lead-off/saturação.
 */
float ad8232_read_ecg_mv();
