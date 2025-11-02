#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>

/**
 * @brief Flag global para controlar se o loop principal deve registrar dados.
 */
extern bool isLoggingActive;

/**
 * @brief Armazena o ID do usuário da sessão atual.
 */
extern char currentUserId[64];

/**
 * @brief Armazena o ID da sessão atual.
 */
extern char currentSessionId[32];

#endif