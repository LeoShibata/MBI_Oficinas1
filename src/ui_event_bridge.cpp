#include "ui_event_bridge.h"
#include "UI/ui.h"
#include "globals.h"        
#include "RTC_Module.h"
#include <Arduino.h>
#include <stdio.h>
#include <string>

void returnButtonLogic(lv_event_t * e) {
    if(isLoggingActive) {
        _ui_screen_change(&ui_Dashboard, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Dashboard_screen_init);
    } else {
        _ui_screen_change(&ui_Tela_Instrumento, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Tela_Instrumento_screen_init);
    }
}

void StartRecordingSession(lv_event_t* e) {
    const char* typedName = lv_textarea_get_text(ui_TextArea2);

    if(strlen(typedName) == 0 || typedName[0] == ' ') {
        Serial.println("ERRO: Nome do paciente obrigatorio!");
        // Pode ser implementado lógica para mudança visual (talvez mudar cor da borda)
        return;
    }

    snprintf(currentUserId, 64, "%s", typedName); // Copia nome para a variável global 

    DateTime now = rtc_getTime();
    snprintf(currentSessionId, 32, "%04d%02d%02d_%02d%02d%02d",
             now.year(), now.month(), now.day(),
             now.hour(), now.minute(), now.second()
            );
        
    isLoggingActive = true;

    Serial.println("=== NOVA SESSAO INICIADA ===");
    Serial.printf("Paciente: %s\n", currentUserId);
    Serial.printf("ID Sessao: %s\n", currentSessionId);

    if(ui_Dashboard) {
        _ui_screen_change(&ui_Dashboard, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Dashboard_screen_init);
    }
}

void StopRecordingSession(lv_event_t * e) {
    if (!isLoggingActive) {
        if(ui_Modo_Registro) _ui_screen_change(&ui_Modo_Registro, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Modo_Registro_screen_init);
        return;
    }

    isLoggingActive = false;
    Serial.println("=== SESSAO FINALIZADA ===");

    if(ui_TextArea2) {
        lv_textarea_set_text(ui_TextArea2, "");
    }

    if(ui_Modo_Registro) {
        _ui_screen_change(&ui_Modo_Registro, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Modo_Registro_screen_init);
    }
}

void setup_ui_logic_bindings() {
    if(ui_BTSalvar) {
        lv_obj_add_event_cb(ui_BTSalvar, StartRecordingSession, LV_EVENT_CLICKED, NULL);
    }

    if(ui_BTSalvar2) {
        lv_obj_add_event_cb(ui_BTSalvar2, StopRecordingSession, LV_EVENT_CLICKED, NULL);
    }

    if(ui_Button6) {
        lv_obj_add_event_cb(ui_Button6, returnButtonLogic, LV_EVENT_CLICKED, NULL);
    }
}