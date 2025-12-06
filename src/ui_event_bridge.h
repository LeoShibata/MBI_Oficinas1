#ifndef UI_EVENT_BRIDGE_H
#define UI_EVENT_BRIDGE_H

#include "lvgl.h"

void returnButtonLogic(lv_event_t * e);
void StartRecordingSession(lv_event_t * e);
void StopRecordingSession(lv_event_t * e);

// Configuração inicial
void setup_ui_logic_bindings();

#endif
