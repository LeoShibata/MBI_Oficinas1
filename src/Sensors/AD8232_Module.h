#pragma once

/**
 * @brief Inicializa o AD8232 (via ADC do ESP32).
 * Por quê: configura pino, atenuação (11 dB) e resolução (12 bits).
 */
void ad8232_init();

/**
 * @brief Lê uma amostra de ECG em milivolts, com baseline removida.
 * @return Valor em mV (positivo/negativo em torno de 0). Retorna -999.f se erro/lead-off.
 * Por quê: entrega o sinal pronto para log/plot sem offset DC de ~Vcc/2.
 */
float ad8232_read_ecg_mv();
