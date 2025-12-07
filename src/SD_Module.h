#ifndef SD_MODULE_H
#define SD_MODULE_H

#include <Arduino.h>

/**
 * @brief Inicializa SD e estrutura /MBI/CSV/(ANO|UNSET).
 * Por quê: preparar diretórios antes de escrever.
 */
void CSV_init();

/**
 * @brief Acrescenta uma linha no CSV atual (arquivo do dia/UNSET).
 * @return false em falha de IO (abrir/gravar).
 */
bool CSV_appendRow(const char* userId,
                   const char* sessionId,
                   const char* sensor,
                   double valor,
                   const char* unidade);

/** @brief Fecha o arquivo atual (flush + close). */
void CSV_close();

/** @brief Caminho do arquivo atual ou NULL. */
const char* CSV_get_current_path(void);

#endif
