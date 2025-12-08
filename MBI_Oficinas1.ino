#include <Wire.h>

#include "src/RTC_Module.h"
#include "src/LVGL_Display.h"
#include "src/SD_Module.h"
#include "src/UI/ui.h"
#include "src/globals.h"
#include "src/ui_event_bridge.h"           

#include "src/Sensors/Temperature_Sensor.h"
#include "src/Sensors/AD8232_Module.h"
#include "src/Sensors/GSR_Module.h"
#include "src/Sensors/MAX30102_Module.h"
#include "src/MAX30102_Processes.h"

// ==========================================
//           VARIÁVEIS GLOBAIS
// ==========================================

// --- Controle de Sessão e Log ---
bool isLoggingActive = false;
char currentUserId[64] = {0};
char currentSessionId[32] = {0};

// --- Controle de Tempo (Timers) ---
unsigned long lastUiUpdate = 0;
const unsigned long uiUpdateInterval = 1000; // Atualiza textos a cada 1s

unsigned long lastGraphUpdate = 0;
const unsigned long graphInterval = 20;      // Atualiza gráficos a cada 20ms

// --- Variáveis de Estado dos Sensores ---
float lastValidGSR = 0.f;

// --- Filtro Visual do Oxímetro (DC Removal) ---
float visual_ir_dc = 0;                      // Para garantir que a onda não fique uma linha reta no topo do gráfico
const float visual_alpha = 0.95;             // Suavização para centro do gráfico 

// --- Variáveis para Cálculo de BPM do ECG ---
unsigned long ecg_last_beat = 0; // Marca o tempo do último batimento
float ecg_bpm_calc = 0.f;        // Guarda o BPM calculado

// ==========================================
//           FUNÇÕES AUXILIARES
// ==========================================

void updateTimeLabelInStatusBar(const char* timeStr) {
    lv_obj_t* statusBar[] = {
        ui_ComBarraStatus1, ui_ComBarraStatus2, ui_ComBarraStatus3, 
        ui_ComBarraStatus4, ui_ComBarraStatus5, ui_ComBarraStatus6, 
        ui_ComStatusBar,    ui_ComStatusBar2,   ui_ComStatusBar3,
        NULL // Para parar o loop 
    };

    for(int i = 0; statusBar[i] != NULL; i++) {
        lv_obj_t* labelHora = ui_comp_get_child(statusBar[i], UI_COMP_COMBARRASTATUS_LABELHORA);
        if(labelHora) {
            lv_label_set_text(labelHora, timeStr);
        }
    }
}

void updateRecDotState() {
    lv_obj_t* statusBar[] = {
        ui_ComBarraStatus1, ui_ComBarraStatus2, ui_ComBarraStatus3, 
        ui_ComBarraStatus4, ui_ComBarraStatus5, ui_ComBarraStatus6, 
        ui_ComStatusBar,    ui_ComStatusBar2,   ui_ComStatusBar3,
        NULL // Para parar o loop 
    };

    for(int i = 0; statusBar[i] != NULL; i++) {
        lv_obj_t* recDot = ui_comp_get_child(statusBar[i], UI_COMP_COMBARRASTATUS_REC_DOT);
        if(recDot) {
            if(isLoggingActive) {
                lv_obj_clear_flag(recDot, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(recDot, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

// ==========================================
//               SETUP
// ==========================================

void setup() {
    Serial.begin(115200);

    // Inicializa I2C (Necessário para o RTC)
    Wire.begin(27, 22); 

    lvgl_display_init();
    ui_init();

    rtc_init();

    Wire.setPins(27, 22);

    CSV_init();
    init_temperature_sensor();
    ad8232_init();
    gsr_init();
    max30102_start();
    ppg_init(100);

    setup_ui_logic_bindings(); 
}

// ==========================================
//           LOOP PRINCIPAL
// ==========================================

void loop() {
    // ------ Gráfico (Tela Oxímetro) ------
    uint32_t red_raw, ir_raw;
    if (max30102_readRaw(&red_raw, &ir_raw)) {

        ppg_feedSample(red_raw, ir_raw); 

        // Processamento VISUAL (Para o Gráfico ficar no meio)
        visual_ir_dc = (visual_alpha * visual_ir_dc) + ((1.0 - visual_alpha) * (float)ir_raw);
        float onda_AC = (float)ir_raw - visual_ir_dc;

        // Atualiza o gráfico (Só se a tela estiver ativa)
        if (ui_Tela_Oximetro && lv_scr_act() == ui_Tela_Oximetro) {
            if(ui_ChartPPG) {
                lv_chart_series_t* ser = lv_chart_get_series_next(ui_ChartPPG, NULL);
                lv_chart_set_next_value(ui_ChartPPG, ser, (int)onda_AC);
            }
        }
    }

    // ------ Gráfico (Tela ECG) ------
    if (millis() - lastGraphUpdate > graphInterval) {
        lastGraphUpdate = millis();

        float mv = ad8232_read_ecg_mv();

        // Para cálculo do BPM
        if (mv > 0.8 && (millis() - ecg_last_beat > 300)) {
             unsigned long delta = millis() - ecg_last_beat;
             
             if(delta > 0) {
                 float inst_bpm = 60000.0 / delta; // 60 segundos / tempo entre batidas
                 
                 // Filtro: Aceita apenas BPMs fisiologicamente possíveis (40 a 220)
                 if(inst_bpm > 40 && inst_bpm < 220) {
                     if(ecg_bpm_calc == 0) {
                        ecg_bpm_calc = inst_bpm;
                     } else {
                        ecg_bpm_calc = (ecg_bpm_calc * 0.9) + (inst_bpm * 0.1);
                    }
                 }
             }
             ecg_last_beat = millis(); // Atualiza o relógio
        }

        if (ui_ECG && lv_scr_act() == ui_ECG) {
            if (mv > -900) { // -999 indica erro de leitura
                int val = 50 + (int)(mv * 20.0f);

                // Limita entre 0 e 100 para não estourar o gráfico
                if (val < 0) {
                    val = 0;
                }
                if (val > 100) {
                    val = 100;
                }
            }
        }
    }

    // ------ Tarefas da UI (LVGL) ------
    lv_timer_handler();
    delay(5);

    // ------ Bloco de Lógica Principal  ------
    unsigned long nowMs = millis();
    if (nowMs - lastUiUpdate >= uiUpdateInterval){
        lastUiUpdate = nowMs;
        
        // ------------ Atualizar Relógio (RTC) ------------
        DateTime now = rtc_getTime();

        // Atualiza label grande da tela Novo Registro
        if(ui_Label10) {
            char rtc_buffer[32];
            sprintf(rtc_buffer, "%02d/%02d/%04d %02d:%02d:%02d",
                now.day(), now.month(), now.year(),
                now.hour(), now.minute(), now.second());
            lv_label_set_text(ui_Label10, rtc_buffer);
        }

        // Atualiza hora nas barras de status (HH:MM)
        char hora_barra_status[6];
        sprintf(hora_barra_status, "%02d:%02d", now.hour(), now.minute());
        updateTimeLabelInStatusBar(hora_barra_status);

        // ------------ Atualizar REC DOT ------------
        updateRecDotState();

        // ------------ Leitura de Sensores (Valores Numéricos) ------------
        // --- Temperatura ---
        float currentTemp = read_temperature();
        char temp_buffer[16];

        if(currentTemp > -100.f) {
            snprintf(temp_buffer, sizeof(temp_buffer), "%.1f", currentTemp);
        } else {
            strcpy(temp_buffer, "--");
        }

        if(ui_Label19) { // Tela Termometro
            lv_label_set_text(ui_Label19, temp_buffer);
        }
        if(ui_LabelTemp) { // Tela Dashboard
            lv_label_set_text(ui_LabelTemp, temp_buffer);
        }

        if(ui_Label21) {
            if(currentTemp <= 35.0) {
                lv_label_set_text(ui_Label21, "Hipotermia");
            } else if(currentTemp > 35.0 && currentTemp <= 37.5) {
                lv_label_set_text(ui_Label21, "Normal");
            } else if(currentTemp > 37.5 && currentTemp <= 39.5) {
                lv_label_set_text(ui_Label21, "Febre");
            } else if(currentTemp > 39.5 && currentTemp < 41) {
                lv_label_set_text(ui_Label21, "Febre alta");
            } else {
                lv_label_set_text(ui_Label21, "Hipertermia");
            }
        }

        // --- GSR (Estresse) ---
        float currentStress = gsr_read_stress();
        char stress_buffer[16];

        if(currentStress > -900.f) {
            lastValidGSR = currentStress;
            snprintf(stress_buffer, sizeof(stress_buffer), "%.1f", lastValidGSR);
        } else {
            strcpy(stress_buffer, "--");
        }

        if(ui_Label27) { // Tela GSR
            lv_label_set_text(ui_Label27, stress_buffer);
        }
        if(ui_LabelGSR) { // Tela Dashboard
            lv_label_set_text(ui_LabelGSR, stress_buffer);
        }

        // --- ECG --- 
        char BPM_buffer[16];
        
        if(ecg_bpm_calc > 1.f) {
            snprintf(BPM_buffer, sizeof(BPM_buffer), "%.0f", ecg_bpm_calc);
        } else {
            strcpy(BPM_buffer, "--");
        }

        if(ui_LabelMedBPM2) {
            lv_label_set_text(ui_LabelMedBPM2, BPM_buffer);
        }

        // --- Oxímetro (SpO2 e BPM) ---
        ppg_tick_1s();
        
        float currentOxiBPM = ppg_getBPM();
        char oxi_bpm_buffer[16];

        if (ppg_hasBPM()) {
            snprintf(oxi_bpm_buffer, sizeof(oxi_bpm_buffer), "%.0f", currentOxiBPM);
        } else {
            strcpy(oxi_bpm_buffer, "--");
        }
        
        if(ui_LabelMedBPM) { // Tela Oxímetro
            lv_label_set_text(ui_LabelMedBPM, oxi_bpm_buffer);
        }
        if(ui_LabelBPM) { // Tela Dashboard
            lv_label_set_text(ui_LabelBPM, oxi_bpm_buffer);
        }

        float currentSpO2 = ppg_getSpO2();
        char SpO2_buffer[16];
        
        if (ppg_hasSpO2()) {
            snprintf(SpO2_buffer, sizeof(SpO2_buffer), "%.0f", currentSpO2);
        } else {
            strcpy(SpO2_buffer, "--");
        }

        if(ui_LabelMedSpO2) {
            lv_label_set_text(ui_LabelMedSpO2, SpO2_buffer);
        }
        if(ui_LabelOxi) {
            lv_label_set_text(ui_LabelOxi, SpO2_buffer);
        }

        // --- Gravação no Cartão SD (Log) ---
        if(isLoggingActive) {
            Serial.printf("LOG: User = %s, Sess = %s, Temp = %.2f C, SpO2 = %.1f\n", currentUserId, currentSessionId, currentTemp, currentSpO2);
            
            // Grava Temperatura
            if (!CSV_appendRow(currentUserId, currentSessionId, "Temperatura", (double)currentTemp, "C")) {
                 Serial.println("Erro: Falha ao gravar Temp no SD.");
            }
            
            // Grava SpO2 (Oxigenação)
            if(ppg_hasSpO2()) {
                 if (!CSV_appendRow(currentUserId, currentSessionId, "SpO2", (double)ppg_getSpO2(), "%")) {
                     Serial.println("Erro: Falha ao gravar SpO2.");
                 }
            }
            
            // Grava GSR (Estresse)
            CSV_appendRow(currentUserId, currentSessionId, "GSR", (double)lastValidGSR, "uS");

            // Grava BPM do ECG 
            if(ecg_bpm_calc > 0) {
                CSV_appendRow(currentUserId, currentSessionId, "BPM (ECG)", (double)ecg_bpm_calc, "bpm");
            }

            // Grava BPM do Oxímetro 
            if(ppg_hasBPM()) {
                CSV_appendRow(currentUserId, currentSessionId, "BPM (Oxi)", (double)currentOxiBPM, "bpm");
            }
        }
    }
}