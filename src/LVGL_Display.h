#ifndef LVGL_DISPLAY_H
#define LVGL_DISPLAY_H

#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <TFT_Touch.h>

static const uint16_t screenWidth = 320;
static const uint16_t screenHeight = 240;

extern TFT_eSPI tft;
extern TFT_Touch touch;

void lvgl_display_init();
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
void my_touchpad_read(lv_indev_drv_t *indev, lv_indev_data_t *data);

#endif