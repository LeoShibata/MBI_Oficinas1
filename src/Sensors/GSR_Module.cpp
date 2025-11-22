#include "GSR_Module.h"
#include <Arduino.h>

// ---- Hardware fixo do divisor ----
static constexpr uint8_t  GSR_ADC_PIN   = 34;        // ADC1, entrada somente
static constexpr float    VCC_VOLTS     = 3.3f;      // Alimentação do divisor
static constexpr float    R_FIXO_OHM    = 220000.0f; // 220 kΩ → corrente << 20 µA (seguro)
static constexpr uint16_t ADC_FULLSCALE = 4095;      // 12 bits

// ---- Estado interno mínimo ----
static bool  g_ready        = false;
static bool  g_has_base     = false;
static float g_base_uS      = 0.0f;   // baseline de condutância (EWMA)
static float g_stress_smooth= 0.0f;   // suavização do nível de estresse

// ---- Parâmetros simples de processamento ----
static constexpr float ALPHA_BASE   = 0.01f; // por quê: baseline lenta p/ acompanhar deriva
static constexpr float ALPHA_STRESS = 0.2f;  // por quê: estabilidade visual no número 0..100
static constexpr float SENS_FRAC    = 0.5f;  // por quê: +50% sobre baseline → ~100 de estresse

// Converte leitura ADC → tensão → resistência da pele → µS.
// Retorna valor válido (>0) ou número negativo em erro.
static float read_microSiemens_once() {
  const int raw = analogRead(GSR_ADC_PIN);
  if (raw <= 0 || raw >= ADC_FULLSCALE) return -1.0f; // saturado/sem contato

  const float v = (static_cast<float>(raw) * VCC_VOLTS) / static_cast<float>(ADC_FULLSCALE);
  if (v <= 0.01f || v >= (VCC_VOLTS - 0.01f)) return -1.0f; // evita div/0 e extremos

  // Divisor: V = Vcc * (Rpele / (Rfixo + Rpele)) ⇒ Rpele = Rfixo * V / (Vcc - V)
  const float r_ohm = R_FIXO_OHM * (v / (VCC_VOLTS - v));
  if (!(r_ohm > 0.0f)) return -1.0f;

  const float microS = 1.0e6f / r_ohm; // G = 1/R (µS)
  // Faixa crua plausível para uso caseiro (evita outliers de contato ruim)
  if (microS < 0.01f || microS > 1000.0f) return -1.0f;

  return microS;
}

void gsr_init() {
  // por quê: 11 dB mede até ~3,3 V → casa com o divisor em 3V3
  analogSetPinAttenuation(GSR_ADC_PIN, ADC_11db);
  analogSetWidth(12);
  pinMode(GSR_ADC_PIN, INPUT);
  g_ready     = true;
  g_has_base  = false;
  g_base_uS   = 0.0f;
  g_stress_smooth = 0.0f;
}

float gsr_read_stress() {
  if (!g_ready) return -999.0f;

  // Leitura única e barata
  const float uS = read_microSiemens_once();
  if (uS <= 0.0f) return -999.0f;

  // Inicializa baseline na 1ª leitura válida
  if (!g_has_base) {
    g_base_uS  = uS;
    g_has_base = true;
    g_stress_smooth = 0.0f; // início em 0
    return 0.0f;
  }

  // Atualiza baseline lentamente (acompanha deriva sem "comer" picos)
  g_base_uS = (1.0f - ALPHA_BASE) * g_base_uS + ALPHA_BASE * uS;

  // Normaliza variação relativa: (uS - base) / (base * sensibilidade)
  const float denom = max(g_base_uS * SENS_FRAC, 1e-6f);
  float stress = (uS - g_base_uS) / denom; // ~1.0 quando +50% acima da base
  // Clampa 0..1.0 e escala para 0..100
  stress = constrain(stress, 0.0f, 1.0f) * 100.0f;

  // Suavização leve para número estável
  g_stress_smooth = (1.0f - ALPHA_STRESS) * g_stress_smooth + ALPHA_STRESS * stress;

  return g_stress_smooth;
}
