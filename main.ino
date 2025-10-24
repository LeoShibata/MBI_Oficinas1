#include <Wire.h>

#include "src/UI/ui.h"              
#include "src/RTC_Module.h"
#include "src/LVGL_Display.h"        

unsigned long lastUpdate = 0;
const unsigned long updateInterval = 1000;  // 1s

void setup() {
    Serial.begin(115200);

    // Inicializa I2C (Necessário para o RTC)
    Wire.begin(27,22); 

    // Inicializa Display, Touch e LVGL
    lvgl_display_init();

    // Inicializa UI
    ui_init();

    // inicializa RTC 
    rtc_init();
}

void loop() {
    lv_timer_handler();  // Processa tarefas LVGL
    delay(5);

    // Atualiza a cada 1 s
    unsigned long nowMs = millis();
    if (nowMs - lastUpdate >= updateInterval){
        lastUpdate = nowMs;

        DateTime now = rtc_getTime();
        char buffer[32];

        sprintf(buffer, "%02d/%02d/%04d %02d:%02d:%02d",
            now.day(), now.month(), now.year(),
            now.hour(), now.minute(), now.second());
        
    // Atualiza label da UI
    lv_label_set_text(ui_Label10, buffer);
    }
}

