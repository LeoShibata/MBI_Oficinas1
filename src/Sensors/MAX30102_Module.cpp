#include <Arduino.h>
#include <Wire.h>
#include <MAX30105.h>
#include "MAX30102_Module.h"

namespace {
  MAX30105 g_sensor;
  bool g_present   = false;
  bool g_streaming = false;
  bool g_shutdown  = false;

  int     g_cfg_sample_rate_sps = 100;
  int     g_cfg_pulse_width_us  = 411;
  uint8_t g_cfg_averaging       = 4;
  uint8_t g_cfg_led_red         = 0x0A;
  uint8_t g_cfg_led_ir          = 0x0A;
}

bool max30102_begin()
{
  g_present   = false;
  g_streaming = false;
  g_shutdown  = false;

  if (!g_sensor.begin(Wire, I2C_SPEED_STANDARD)) {
    return false;
  }

  g_sensor.setup();
  g_sensor.setLEDMode(2);
  g_sensor.setFIFOAverage(g_cfg_averaging);
  g_sensor.setSampleRate(g_cfg_sample_rate_sps);
  g_sensor.setPulseWidth(g_cfg_pulse_width_us);
  g_sensor.setPulseAmplitudeRed(g_cfg_led_red);
  g_sensor.setPulseAmplitudeIR(g_cfg_led_ir);
  g_sensor.clearFIFO();

  g_present = true;
  return true;
}

bool max30102_available()
{
  if (!g_present) return false;
  g_sensor.check();
  return g_sensor.available();
}

bool max30102_readRaw(uint32_t* red, uint32_t* ir)
{
  if (!red || !ir) return false;
  if (!g_present)  return false;
  if (!g_sensor.available()) return false;
  *red = g_sensor.getRed();
  *ir  = g_sensor.getIR();
  g_sensor.nextSample();
  return true;
}

bool max30102_start()
{
  if (!g_present) return false;
  if (g_streaming) return true;
  if (g_shutdown) { g_sensor.wakeUp(); delay(10); }
  g_sensor.clearFIFO();
  g_sensor.setLEDMode(2);
  g_sensor.setFIFOAverage(g_cfg_averaging);
  g_sensor.setSampleRate(g_cfg_sample_rate_sps);
  g_sensor.setPulseWidth(g_cfg_pulse_width_us);
  g_sensor.setPulseAmplitudeRed(g_cfg_led_red);
  g_sensor.setPulseAmplitudeIR(g_cfg_led_ir);
  g_streaming = true;
  g_shutdown  = false;
  return true;
}

bool max30102_stop()
{
  if (!g_present) return false;
  if (!g_streaming && g_shutdown) return true;
  g_sensor.shutDown();
  g_streaming = false;
  g_shutdown  = true;
  return true;
}

bool max30102_isPresent()
{
  return g_present;
}

