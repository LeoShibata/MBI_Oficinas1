#ifndef SD_MODULE_H
#define SD_MODULE_H

#include <Arduino.h>

/**
 * Inicializa o SD e garante a estrutura /MBI/CSV/(ANO|UNSET).
 * Idempotente no sentido de "pode chamar no setup". Mantém a API simples.
 */
void CSV_init();

/**
 * Acrescenta uma linha no CSV atual (arquivo do dia ou UNSET).
 * Retorna false se falhar abrir/gravar.
 */
bool CSV_appendRow(const char* userId,
                   const char* sessionId,
                   const char* sensor,
                   double valor,
                   const char* unidade);

/** Fecha (flush + close) o arquivo atual, se aberto. */
void CSV_close();

/** Retorna o caminho do arquivo atual, ou NULL se indisponível. */
const char* CSV_get_current_path(void);

#endif