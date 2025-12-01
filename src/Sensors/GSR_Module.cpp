#include "GSR_Module.h"
#include <Arduino.h>

// Divisor fixo (moedas ↔ pele ↔ nó → ADC35; do nó também vai um resistor de 1 MΩ para 3V3)
static constexpr uint8_t  GSR_ADC_PIN   = 35;         // ADC1_CH7 (só entrada)
static constexpr float    VCC_VOLTS     = 3.3f;
static constexpr float    R_FIXO_OHM    = 1000000.0f; // 1 MΩ
static constexpr uint16_t ADC_FULLSCALE = 4095;       // 12 bits

// Estado
static bool  g_ready         = false;
static bool  g_has_base      = false;
static float g_base_uS       = 0.0f;   // baseline em µS (EWMA)
static float g_stress_smooth = 0.0f;   // saída suavizada 0..100

// Parâmetros de processamento
static constexpr float ALPHA_BASE   = 0.01f; // baseline lenta
static constexpr float ALPHA_STRESS = 0.2f;  // suavização de output
static constexpr float SENS_FRAC    = 0.5f;  // +50% sobre base → ~100

static float read_microSiemens_once() {
  const int raw = analogRead(GSR_ADC_PIN);
  if (raw <= 0 || raw >= ADC_FULLSCALE) return -1.0f;

  const float v = (static_cast<float>(raw) * VCC_VOLTS) / static_cast<float>(ADC_FULLSCALE);
  if (v <= 0.01f || v >= (VCC_VOLTS - 0.01f)) return -1.0f;

  const float r_ohm = R_FIXO_OHM * (v / (VCC_VOLTS - v)); // divisor
  if (!(r_ohm > 0.0f)) return -1.0f;

  const float microS = 1.0e6f / r_ohm; // G = 1/R (µS)
  if (microS < 0.01f || microS > 1000.0f) return -1.0f; // descarta outliers
  return microS;
}

void gsr_init() {
#if defined(ARDUINO_ARCH_ESP32)
  analogSetPinAttenuation(GSR_ADC_PIN, ADC_11db); // mede até ~3.3 V
  analogSetWidth(12);
#endif
  pinMode(GSR_ADC_PIN, INPUT);
  g_ready = true;
  g_has_base = false;
  g_base_uS = 0.0f;
  g_stress_smooth = 0.0f;
}

float gsr_read_stress() {
  if (!g_ready) return -999.0f;

  const float uS = read_microSiemens_once();
  if (uS <= 0.0f) return -999.0f;

  if (!g_has_base) {
    g_base_uS  = uS;      // inicia baseline na 1ª leitura válida
    g_has_base = true;
    g_stress_smooth = 0.0f;
    return 0.0f;
  }

  // Atualiza baseline lentamente
  g_base_uS = (1.0f - ALPHA_BASE) * g_base_uS + ALPHA_BASE * uS;

  // Normaliza variação relativa para 0..100
  const float denom = max(g_base_uS * SENS_FRAC, 1e-6f);
  float stress = (uS - g_base_uS) / denom;
  stress = constrain(stress, 0.0f, 1.0f) * 100.0f;

  // Suaviza saída
  g_stress_smooth = (1.0f - ALPHA_STRESS) * g_stress_smooth + ALPHA_STRESS * stress;
  return g_stress_smooth;

}

float gsr_read_microSiemens() {
  // usa a mesma leitura interna do módulo; retorna <0 se inválido
  extern float read_microSiemens_once(); // se read_microSiemens_once for 'static', remova 'extern' e chame direto
  return read_microSiemens_once();
}
