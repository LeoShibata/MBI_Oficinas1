#ifndef UI_EVENT_BRIDGE_H
#define UI_EVENT_BRIDGE_H

#include "lvgl.h"

// Funções chamadas pelos botões da interface
void StartRecordingSession(lv_event_t * e);
void StopRecordingSession(lv_event_t * e);

// Configuração inicial
void setup_ui_logic_bindings();

#endif
