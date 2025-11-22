#pragma once
#include <stdint.h>

/**
 * @brief Inicializa o processador PPG.
 * @param sample_rate_sps Taxa de amostragem (ex.: 100).
 */
void ppg_init(int sample_rate_sps);

/** @brief Zera todos os estados internos (DC, acumuladores, buffers, BPM/SpO2). */
void ppg_reset();

/**
 * @brief Define um nível mínimo de DC no IR para considerar que há dedo em contato.
 * @param ir_dc_min Valor DC mínimo (em contagens do ADC).
 * Por quê: evita falso BPM/SpO2 sem acoplamento.
 */
void ppg_setFingerThreshold(uint32_t ir_dc_min);

/**
 * @brief Alimenta o processador com uma amostra bruta.
 * @param red Valor RED bruto.
 * @param ir  Valor IR bruto.
 */
void ppg_feedSample(uint32_t red, uint32_t ir);

/**
 * @brief Chamado 1 vez por segundo para consolidar métricas (R, SpO2, BPM).
 */
void ppg_tick_1s();

/** @brief Indica se há SpO2 válido calculado. */
bool  ppg_hasSpO2();
/** @brief SpO2 (0–100). Válido somente se ppg_hasSpO2()==true. */
float ppg_getSpO2();

/** @brief Indica se há BPM válido calculado. */
bool  ppg_hasBPM();
/** @brief BPM atual (30–220). Válido somente se ppg_hasBPM()==true. */
float ppg_getBPM();
