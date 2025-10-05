#include <lvgl.h>
#include <TFT_eSPI.h>          
#include <TFT_Touch.h> 

#include <Wire.h>
#include <RTClib.h>

#include "ui.h"              

// --------------------- Configurações do Display ---------------------
static const uint16_t screenWidth = 320;
static const uint16_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * screenHeight / 4];

TFT_eSPI tft = TFT_eSPI();  
RTC_DS3231 rtc;            

unsigned long lastUpdate = 0;
const unsigned long updateInterval = 1000;  // 1s

// --------------------- Configurações do Touch ---------------------
#define DOUT 39  /* Data out pin (T_DO)    */
#define DIN  32  /* Data in pin (T_DIN)    */
#define DCS  33  /* Chip select pin (T_CS) */
#define DCLK 25  /* Clock pin (T_CLK)      */

TFT_Touch touch = TFT_Touch(DCS, DCLK, DIN, DOUT);

// --------------------- Função de Flush do Display ---------------------
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

// --------------------- Função de Leitura do Touch para LVGL ---------------------
void my_touchpad_read(lv_indev_drv_t *indev, lv_indev_data_t *data) {
    uint16_t x, y;

    if (!touch.Pressed()) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        x = touch.X();
        y = touch.Y();     
        
        data->state = LV_INDEV_STATE_PR;
        data->point.x = x;
        data->point.y = y;
    }
}

// --------------------- Setup ---------------------
void setup() {
    Serial.begin(115200);

    // Inicializa display TFT
    tft.begin();
    tft.setRotation(1);  // 0=portrait, 1=landscape
    tft.fillScreen(TFT_BLACK);

    // Inicializa touch com calibração
    touch.setCal(526, 3443, 750, 3377, screenWidth, screenHeight, 1);

    // Inicializa LVGL
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);

    // Driver de display LVGL
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // Drive de input (touch)
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    // Inicializa UI
    ui_init();

    // inicializa RTC 
    if (!rtc.begin()){
        Serial.println("Erro: RTC não encontrado!");
    } else {
        if (rtc.lostPower()){
            Serial.println("RTC sem hora. Ajustando hora para compilação.");
            rtc.adjust(DataTime(F(__DATA__), F(__TIME__)));  // Ajuste inicial
        }
    }
}

// --------------------- Loop ---------------------
void loop() {
    lv_timer_handler();  // Processa tarefas LVGL
    delay(5);

    // Atualiza a cada 1 s
    unsigned long nowMs = millis();
    if (nowMs - lastUpdate >= updateInterval){
        lastUpdate = nowMs;

        DateTime now = rtc.now();
        char buffer[32];

        sprintf(buffer, "%02d/%02d/%04d %02d:%02d:%02d",
            now.day(), now.month(), now.year(),
            now.hour(), now.minute(), now.second());
        
    // Atualiza label da UI
    lv_label_set_text(ui_label10, buffer);
    }
}

