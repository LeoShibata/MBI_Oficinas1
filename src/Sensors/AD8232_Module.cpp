#include "AD8232_Module.h"
#include <Arduino.h>

// Pinos/ADC
static constexpr uint8_t  ECG_ADC_PIN   = 35;      // ADC1 (entrada)
static constexpr float    VCC_VOLTS     = 3.3f;
static constexpr uint16_t ADC_FULLSCALE = 4095;

// Lead-off opcional (coloque -1 se não usar)
static constexpr int LO_PLUS_PIN  = -1;
static constexpr int LO_MINUS_PIN = -1;

static constexpr float ALPHA_BASE = 0.01f; // baseline lenta

// Estado
static bool  g_ready    = false;
static bool  g_has_base = false;
static float g_base_mv  = 0.0f;
static float g_last_mv  = 0.0f;

static inline bool lead_off_active() {
  bool lo = false;
  if (LO_PLUS_PIN  >= 0) lo |= (digitalRead(LO_PLUS_PIN)  == HIGH);
  if (LO_MINUS_PIN >= 0) lo |= (digitalRead(LO_MINUS_PIN) == HIGH);
  return lo;
}

static inline int read_raw_adc() { return analogRead(ECG_ADC_PIN); }

static float read_millivolts() {
#if defined(ARDUINO_ARCH_ESP32)
  return (float)analogReadMilliVolts(ECG_ADC_PIN); // usa calibração do ESP32
#else
  int raw = read_raw_adc();
  return (float)raw * (VCC_VOLTS * 1000.0f) / (float)ADC_FULLSCALE;
#endif
}

void ad8232_init() {
  analogSetPinAttenuation(ECG_ADC_PIN, ADC_11db); // ~0..3.3 V
  analogSetWidth(12);
  pinMode(ECG_ADC_PIN, INPUT);
  if (LO_PLUS_PIN  >= 0) pinMode(LO_PLUS_PIN,  INPUT);
  if (LO_MINUS_PIN >= 0) pinMode(LO_MINUS_PIN, INPUT);

  g_ready = true;
  g_has_base = false;
  g_base_mv = 0.0f;
  g_last_mv = 0.0f;
  delay(20); // estabilização breve
}

float ad8232_read_ecg_mv() {
  if (!g_ready) return -999.0f;
  if (lead_off_active()) return -999.0f;

  const float mv = read_millivolts();
  g_last_mv = mv;

  // Saturações óbvias
  if (mv <= 1.0f || mv >= (VCC_VOLTS * 1000.0f - 1.0f)) return -999.0f;

  if (!g_has_base) {
    g_base_mv = mv;
    g_has_base = true;
    return 0.0f; // primeira saída centrada
  }

  // Remove offset DC por EWMA lenta
  g_base_mv = (1.0f - ALPHA_BASE) * g_base_mv + ALPHA_BASE * mv;
  const float ecg_mv = mv - g_base_mv;

  // Proteção básica contra outliers absurdos
  if (ecg_mv < -2000.0f || ecg_mv > 2000.0f) return -999.0f;

  return ecg_mv;
}
