#pragma once
#include <stdint.h>

/**
 * @brief Inicializa o MAX30102 com parâmetros validados (SpO2).
 * Por quê: 400 kHz, 100 sps, 411 us, AVG=4, ADC=16384, IR/RED=0x1F.
 */
bool max30102_begin();

/** @brief Inicia/Reaplica configuração e limpa FIFO (idempotente). */
bool max30102_start();

/** @brief Coloca o sensor em shutdown (baixa energia). */
bool max30102_stop();

/** @brief Há amostras disponíveis na FIFO? */
bool max30102_available();

/**
 * @brief Lê 1 amostra RED/IR da FIFO (sincronizada).
 * @return true em sucesso; false se indisponível.
 */
bool max30102_readRaw(uint32_t* red, uint32_t* ir);

/** @brief Sensor presente desde o begin()? */
bool max30102_isPresent();
