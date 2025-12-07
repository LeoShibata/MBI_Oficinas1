#include <Arduino.h>
#include <Wire.h>
#include <MAX30105.h>          // Lib SparkFun usada no projeto; suporta MAX30102
#include "MAX30102_Module.h"

namespace {
  MAX30105 g_sensor; // objeto SparkFun (cobre MAX30102)

  bool g_present   = false;
  bool g_streaming = false;
  bool g_shutdown  = false;

  constexpr int      CFG_SAMPLE_RATE_SPS = 100;
  constexpr int      CFG_PULSE_WIDTH_US  = 411;
  constexpr uint8_t  CFG_AVERAGING       = 4;
  constexpr uint16_t CFG_ADC_RANGE       = 16384;
  constexpr uint8_t  CFG_LED_RED         = 0x1F;
  constexpr uint8_t  CFG_LED_IR          = 0x1F;
  constexpr uint8_t  CFG_LED_MODE        = 2;   // IR + RED
}

static void apply_config_steady() {
  // por quê: aplica conjunto validado para estabilidade
  g_sensor.setLEDMode(CFG_LED_MODE);
  g_sensor.setFIFOAverage(CFG_AVERAGING);
  g_sensor.setSampleRate(CFG_SAMPLE_RATE_SPS);
  g_sensor.setPulseWidth(CFG_PULSE_WIDTH_US);
  g_sensor.setADCRange(CFG_ADC_RANGE);

  g_sensor.setPulseAmplitudeRed(CFG_LED_RED);
  g_sensor.setPulseAmplitudeIR(CFG_LED_IR);
  g_sensor.setPulseAmplitudeGreen(0x00); // desliga green

  g_sensor.enableFIFORollover();         // evita travar em loop lento
  g_sensor.clearFIFO();                  // limpa lixo inicial
}

bool max30102_begin() {
  g_present   = false;
  g_streaming = false;
  g_shutdown  = false;

  Wire.setClock(400000);                 // por quê: melhor resposta do FIFO/AC
  if (!g_sensor.begin(Wire, I2C_SPEED_FAST)) {
    return false;
  }

  g_sensor.softReset();
  delay(10);
  g_sensor.setup();
  apply_config_steady();

  g_present = true;
  return true;
}

bool max30102_start() {
  if (!g_present)  return false;
  if (g_streaming) return true;
  if (g_shutdown) { g_sensor.wakeUp(); delay(10); }

  apply_config_steady();
  g_streaming = true;
  g_shutdown  = false;
  return true;
}

bool max30102_stop() {
  if (!g_present) return false;
  if (!g_streaming && g_shutdown) return true;
  g_sensor.shutDown();
  g_streaming = false;
  g_shutdown  = true;
  return true;
}

bool max30102_available() {
  if (!g_present) return false;
  g_sensor.check();
  return g_sensor.available();
}

bool max30102_readRaw(uint32_t* red, uint32_t* ir) {
  if (!red || !ir) return false;
  if (!g_present)  return false;
  if (!g_sensor.available()) return false;

  *red = g_sensor.getFIFORed();
  *ir  = g_sensor.getFIFOIR();
  g_sensor.nextSample();
  return true;
}

bool max30102_isPresent() { return g_present; }
