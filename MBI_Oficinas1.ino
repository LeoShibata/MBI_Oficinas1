#include <Wire.h>

#include "src/RTC_Module.h"
#include "src/LVGL_Display.h"
#include "src/SD_Module.h"
#include "src/Sensors/Temperature_Sensor.h"
#include "src/Sensors/AD8232_Module.h"
#include "src/Sensors/GSR_Module.h"
#include "src/MAX30102_Processes.h"

#include "src/UI/ui.h"
#include "src/globals.h"
#include "src/ui_event_bridge.h"           

//---------Variáveis Globais---------
bool isLoggingActive = false;
char currentUserId[64] = {0};
char currentSessionId[32] = {0};
//-----------------------------------

float lastValidGSR = 0.f;

unsigned long lastUpdate = 0;
const unsigned long updateInterval = 1000;  // 1s

unsigned long lastGraphUpdate = 0;
const unsigned long graphInterval = 20; 

void setup() {
    Serial.begin(115200);

    // Inicializa I2C (Necessário para o RTC)
    Wire.begin(27,22); 

    lvgl_display_init();
    ui_init();
    rtc_init();
    CSV_init();
    init_temperature_sensor();
    ad8232_init();
    gsr_init();
}

void loop() {

// -----------------------------------------------------------------
    // 1. ATUALIZAÇÃO RÁPIDA (GRÁFICOS) - Executa a cada ~20ms
    // -----------------------------------------------------------------
    if (millis() - lastGraphUpdate > graphInterval) {
        lastGraphUpdate = millis();

        // --- Gráfico de ECG (Tela: ui_ECG, Chart: ui_Chart2) ---
        // Verifica se a tela de ECG está ativa e se o gráfico existe
        if (ui_Chart2 && lv_scr_act() == ui_ECG) {
            float mv = ad8232_read_ecg_mv();
            
            // Se leitura válida (ad8232 retorna -999 se erro)
            if (mv > -900) {
                // Escalonamento simples para caber no gráfico (0-100)
                // Assumindo que o sinal varia aprox +/- 2.0mV. 
                // Centro em 50. Ganho de 20x.
                int val = 50 + (int)(mv * 20.0f);
                
                // Limita entre 0 e 100 para não estourar o gráfico
                if (val < 0) val = 0;
                if (val > 100) val = 100;

                // Adiciona o ponto à série do gráfico
                lv_chart_series_t* ser = lv_chart_get_series_next(ui_Chart2, NULL);
                lv_chart_set_next_value(ui_Chart2, ser, val);
            }
        }
    }

    lv_timer_handler();  // Processa tarefas LVGL (eventos, animações)
    delay(5);

    // Bloco de Lógica Principal (executa a cada 'updateInterval')
    unsigned long nowMs = millis();
    if (nowMs - lastUpdate >= updateInterval){
        lastUpdate = nowMs;
        
        // Atualiza Relógio (Tela Novo_Registro)
        DateTime now = rtc_getTime();
        char rtc_buffer[32];
        sprintf(rtc_buffer, "%02d/%02d/%04d %02d:%02d:%02d",
            now.day(), now.month(), now.year(),
            now.hour(), now.minute(), now.second());
        
        // Atualiza label da UI (se tela estiver carregada)
        if(ui_Label10) {
            lv_label_set_text(ui_Label10, rtc_buffer);
        }


        // ---------------- Temperatura ----------------
        // Lê a temperatura uma vez por ciclo
        float currentTemp = read_temperature();

        // Formata para a tela do Termômetro
        char temp_buffer[16];
        snprintf(temp_buffer, sizeof(temp_buffer), "%.1f", currentTemp);

        // Atualiza label da UI (se tela estiver carregada)
        if(ui_Label19) {
            lv_label_set_text(ui_Label19, temp_buffer);
        }
        

        // ---------------- GSR ----------------
        // Lê GSR uma vez por ciclo
        float currentStress = gsr_read_stress();

        if(currentStress > -900.f) {
            lastValidGSR = currentStress;
        }

        // Formata para a tela do Termômetro
        char stress_buffer[16];
        snprintf(stress_buffer, sizeof(stress_buffer), "%.1f", lastValidGSR);

        // Atualiza label da UI (se tela estiver carregada)
        if(ui_Label27) {
            lv_label_set_text(ui_Label27, stress_buffer);
        }


        // ---------------- ECG ----------------
        // Lê ECG uma vez por ciclo
        float currentBPM = ad8232_read_ecg_mv();

        // Formata para a tela do Termômetro
        char BPM_buffer[16];
        snprintf(BPM_buffer, sizeof(BPM_buffer), "%.1f", currentBPM);

        // Atualiza label da UI (se tela estiver carregada)
        if(ui_Label27) {
            lv_label_set_text(ui_LabelMedBPM2, BPM_buffer);
        }


        // ---------------- OXIMETRO ----------------
        // Lê cpm do oximetro uma vez por ciclo
        float currentOxiBPM = ppg_getBPM();

        // Formata para a tela do Termômetro
        char OxiBPM_buffer[16];
        snprintf(OxiBPM_buffer, sizeof(OxiBPM_buffer), "%.1f", currentOxiBPM);

        // Atualiza label da UI (se tela estiver carregada)
        if(ui_LabelMedBPM) {
            lv_label_set_text(ui_LabelMedBPM, OxiBPM_buffer);
        }

        // Lê SpO2 uma vez por ciclo
        float currentSpO2 = ppg_getSpO2();

        // Formata para a tela do Termômetro
        char SpO2_buffer[16];
        snprintf(SpO2_buffer, sizeof(SpO2_buffer), "%.1f", currentSpO2);

        // Atualiza label da UI (se tela estiver carregada)
        if(ui_LabelMedSpO2) {
            lv_label_set_text(ui_LabelMedBPM, SpO2_buffer);
        }

        // ------------------------------------------

        // Lógica de registro de dados 
        if(isLoggingActive) {
            Serial.printf("LOG: User=%, Sess=%s, Temp=%2.f C\n",
                          currentUserId, currentSessionId, currentTemp);
            
            // Salva a linha no arquivo CSV usando o valor já lido.
            bool success = CSV_appendRow(currentUserId,
                                         currentSessionId,
                                         "Temperatura",
                                         (double)currentTemp,
                                         "C");
            
            if(!success) {
                Serial.println("Erro: Falha ao gravar no Sd Card.");
            }
        }
    }
}