#include "AD8232_Module.h"
#include <Arduino.h>

// ---------------- Parâmetros mínimos (ajuste fácil) ----------------
// Por quê: GPIO35 é ADC1 (entrada apenas), estável e sem conflito com WiFi.
static constexpr uint8_t  ECG_ADC_PIN     = 35;
static constexpr float    VCC_VOLTS       = 3.3f;     // trilho de 3V3
static constexpr uint16_t ADC_FULLSCALE   = 4095;     // 12 bits

// Pinos opcionais de detecção de "lead-off" (alto = eletrodo solto).
// Coloque -1 para desabilitar se não estiver usando o LO do breakout.
static constexpr int LO_PLUS_PIN  = -1;
static constexpr int LO_MINUS_PIN = -1;

// Filtro de baseline (EWMA) para remover o offset ~Vcc/2 do AD8232.
static constexpr float ALPHA_BASE = 0.01f;  // por quê: segue deriva lenta sem "comer" QRS

// ---------------- Estado interno ----------------
static bool  g_ready      = false;
static bool  g_has_base   = false;
static float g_base_mv    = 0.0f;   // baseline em milivolts
static float g_last_mv    = 0.0f;   // última amostra absoluta em mV (debug se quiser)

// ---------------- Helpers ----------------
static inline bool lead_off_active() {
  // por quê: se pinos LO estiverem definidos, usa como guarda de contato
  bool lo = false;
  if (LO_PLUS_PIN  >= 0) lo |= (digitalRead(LO_PLUS_PIN)  == HIGH);
  if (LO_MINUS_PIN >= 0) lo |= (digitalRead(LO_MINUS_PIN) == HIGH);
  return lo;
}

static inline int read_raw_adc() {
  return analogRead(ECG_ADC_PIN);
}

static float read_millivolts() {
#if defined(ARDUINO_ARCH_ESP32)
  // por quê: usa calibração de fábrica do ESP32 para mV mais realistas
  return (float)analogReadMilliVolts(ECG_ADC_PIN);
#else
  // Fallback genérico (se portar para outro core): mapeia linearmente
  int raw = read_raw_adc();
  return (float)raw * (VCC_VOLTS * 1000.0f) / (float)ADC_FULLSCALE;
#endif
}

// ---------------- API ----------------
void ad8232_init() {
  // Configura ADC: 11 dB mede ~0..3.3 V, 12 bits para melhor resolução
  analogSetPinAttenuation(ECG_ADC_PIN, ADC_11db);
  analogSetWidth(12);
  pinMode(ECG_ADC_PIN, INPUT);

  // Pinos de lead-off (opcionais)
  if (LO_PLUS_PIN  >= 0) pinMode(LO_PLUS_PIN,  INPUT);
  if (LO_MINUS_PIN >= 0) pinMode(LO_MINUS_PIN, INPUT);

  // Reset de estado
  g_ready    = true;
  g_has_base = false;
  g_base_mv  = 0.0f;
  g_last_mv  = 0.0f;

  // Pequena estabilização do front-end
  delay(20); // por quê: evita primeira amostra enviesada logo ao ligar
}

float ad8232_read_ecg_mv() {
  if (!g_ready) return -999.0f;
  if (lead_off_active()) return -999.0f; // por quê: eletrodo solto → não confie no valor

  // Ler tensão absoluta em mV no pino do ADC
  const float mv = read_millivolts();
  g_last_mv = mv;

  // Saturações óbvias (0 ou VCC) indicam erro/ruído severo
  if (mv <= 1.0f || mv >= (VCC_VOLTS * 1000.0f - 1.0f)) {
    return -999.0f;
  }

  // Inicializa baseline na primeira leitura válida
  if (!g_has_base) {
    g_base_mv  = mv;
    g_has_base = true;
    return 0.0f; // por quê: primeira saída centrada em 0
  }

  // Atualiza baseline lentamente (remove offset DC do AD8232)
  g_base_mv = (1.0f - ALPHA_BASE) * g_base_mv + ALPHA_BASE * mv;

  // Output centrado (mV relativos): positivo/negativo em torno de 0
  const float ecg_mv = mv - g_base_mv;

  // Clampe leve contra outliers absurdos (proteção básica)
  if (ecg_mv < -2000.0f || ecg_mv > 2000.0f) return -999.0f;

  return ecg_mv;
}
