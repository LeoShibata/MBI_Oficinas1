#include "ui_event_bridge.h"
#include "globals.h"
#include "RTC_Module.h"
#include "SD_Module.h"
#include <stdio.h>
#include <string>
#include <SD.h>

#include "lvgl.h"

// Função auxiliar interna para pegar o tempo do RTC de forma segura
static DataTime rtc_now_safe_cpp(bool* ok = nullptr) {
    DateTime t = rtc_getTime();
    // Checagem de sanidade simples
    bool valid = (t.year() >= 2020 && t.year() <= 2029 &&
                  t.month() >= 1 && t.month() <= 12 && 
                  t.day() >= 1 && t.day() <= 31);
    if(ok)
        *ok = valid;
    if(!valid)
        return DateTime(2000, 1, 1, 0, 0, 0);
    return t;
}

extern "C" void app_start_logging_session(const char* userId) {
    (isLoggingActive) {
        // Já estava logado
        app_stop_logging_session();
    }

    if(!userIde || *userId == '\0') {
        Serial.println("[Event_Bridge] Nao pode iniciar log: UserId está vazio.");
        return;
    }

    // Armazena o UserID
    strncpy(currentUserId, userId, sizeof(currentUserId) - 1);
    currentUserId[sizeof(currentUserId) - 1] = '\0';

    // Cria um SessionID unico a partir do timestamp do RTC
    bool rtc_ok = false;
    DateTime now = rtc_now_safe_cpp(&rtc_ok);
    if(rtc_ok) {
        snprintf(currentSessionId, sizeof(currentSessionId),
                "%04d%02d%02d-%02d%02d%02d",
                now.year(), now.month(), now.day(),
                now.hour(), now.minute(), now.second());
    } else {
        snprintf(currentSessionId, sizeof(currentSessionId), "NO_RTC-%lu", (unsigned long)millis());
    }
    
    // Garante que o arquivo SD do dia está aberto e pronto
    if(!openDailyIfNeeded()) {
        Serial.println("[Event_Bridge] ERRO: Falha ao abrir arquivo SD para iniciar sessao.");
        currentUserId[0] = '\0';
        currentSessionId[0] = '\0';
        isLoggingActive = false;
        return;
    }

    // Seta a flag para o loop() começar a gravar
    isLoggingActive = true;
    Serial.print("[Event_Bridge] Sessao de log iniciada. UserID=");
    Serial.print(currentUserId);
    Serial.print(", SessionID=");
    Serial.println(currentSessionId);
}

// Implementação da função C que será chamada pela UI
extern "C" void app_stop_logging_session(void) {
    if(!isLoggingActive) {
        return; // Nada a fazer
    }
    
    isLoggingActive = false;
    Serial.print("[Event_Bridge] Sessao de log parada. UserID=");
    Serial.print(currentUserId);
    Serial.print(", SessionID=");
    Serial.println(currentSessionId);
    
    // Limpa os dados da sessão
    currentUserId[0] = '\0';
    currentSessionId[0] = '\0';
}

// Implementação da função C que será chamada pela UI
extern "C" void app_load_data_preview(lv_obj_t* label) {
    if (!label) 
        return;

    // Pega o caminho do arquivo atual do módulo SD
    const char* path = CSV_get_current_path();
    
    if (!path || !*path) {
        char fallback_path[64];
        build_paths_safe(nullptr, 0, fallback_path, sizeof(fallback_path), nullptr, nullptr);
        path = fallback_path;
        
        if (!SD.exists(path)) {
            lv_label_set_text(label, "Nenhum registro encontrado para hoje.");
            return;
        }
    }

    File f = SD.open(path, FILE_READ);
    if (!f) {
        char err_msg[128];
        snprintf(err_msg, sizeof(err_msg), "Erro: Nao foi possivel abrir:\n%s", path);
        lv_label_set_text(label, err_msg);
        return;
    }

    if (f.size() == 0) {
        lv_label_set_text(label, "Arquivo de hoje esta vazio.");
        f.close();
        return;
    }

    // Lê as primeiras 5 linhas de dados (mais 1 de header)
    std::string preview_text = "Preview (Primeiras 6 linhas):\n";
    int line_count = 0;
    while(f.available() && line_count < 6) {
        String line_str = f.readStringUntil('\n');
        line_str.trim(); // Remove \r ou \n
        if(line_str.length() > 0) {
            preview_text += line_str.c_str();
            preview_text += "\n";
            line_count++;
        }
    }
    f.close();
    
    // Define o texto no label da UI
    lv_label_set_text(label, preview_text.c_str());
}
