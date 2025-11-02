#include <Wire.h>

#include "src/UI/ui.h"              
#include "src/RTC_Module.h"
#include "src/LVGL_Display.h"
#include "src/SD_Module.h"

#include "src/Sensors/Temperature_Sensor.h"

unsigned long lastUpdate = 0;
const unsigned long updateInterval = 1000;  // 1s

void setup() {
    Serial.begin(115200);

    // Inicializa I2C (Necessário para o RTC)
    Wire.begin(27,22); 

    lvgl_display_init();
    ui_init();
    rtc_init();
    init_temperature_sensor();
}

void loop() {
    lv_timer_handler();  // Processa tarefas LVGL (eventos, animações)
    delay(5);

    // Bloco de Lógica Principal (executa a cada 'updateInterval')
    unsigned long nowMs = millis();
    if (nowMs - lastUpdate >= updateInterval){
        lastUpdate = nowMs;
        
        // Atualiza Relógio (Tela Novo_Registro)
        DateTime now = rtc_getTime();
        char rtc_buffer[32];
        sprintf(buffer, "%02d/%02d/%04d %02d:%02d:%02d",
            now.day(), now.month(), now.year(),
            now.hour(), now.minute(), now.second());
        
        // Atualiza label da UI (se tela estiver carregada)
        if(ui_Label10) {
            lv_label_set_text(ui_Label10, buffer);
        }

        // Lê a temperatura uma vez por ciclo
        float currentTemp = read_temperature();

        // Formata para a tela do Termômetro
        char temp_buffer[16];
        snprintf(temp_buffer, sizeof(temp_buffer), "%.1f °C", currentTemp);

        if(ui_Label20) {
            lv_label_set_text(ui_Label20, temp_buffer);
        }
    }
}
