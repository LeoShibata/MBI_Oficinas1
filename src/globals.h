#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>

// Variáveis Globais Compartilhadas
// "extern" avisa ao compilador que essas variáveis existem no .ino

extern bool isLoggingActive;      // true = Gravando
extern char currentUserId[64];    // Nome do Paciente
extern char currentSessionId[32]; // ID da Sessão (DataHora)

#endif