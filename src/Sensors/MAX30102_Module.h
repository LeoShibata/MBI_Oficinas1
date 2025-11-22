#pragma once
#include <stdint.h>

/**
 * @brief Inicializa o MAX30102 com os parâmetros validados (SpO2).
 * Por quê: aplica 400 kHz, 100 sps, 411 us, AVG=4, ADC=16384, IR/RED=0x1F.
 */
bool max30102_begin();

/**
 * @brief Inicia ou reconfigura o streaming (modo 2: IR+RED).
 * Idempotente.
 */
bool max30102_start();

/**
 * @brief Coloca o sensor em shutdown (menor consumo).
 * Idempotente.
 */
bool max30102_stop();

/**
 * @brief Há amostras disponíveis na FIFO?
 */
bool max30102_available();

/**
 * @brief Lê 1 amostra RED/IR da FIFO (sincronizada).
 * @return true em sucesso; false se indisponível.
 */
bool max30102_readRaw(uint32_t* red, uint32_t* ir);

/**
 * @brief O sensor foi detectado com sucesso no begin()?
 */
bool max30102_isPresent();
