#include <Arduino.h>
#include <Wire.h>
#include <MAX30105.h>          // Biblioteca SparkFun já utilizada no projeto
#include "MAX30102_Module.h"

namespace {
  // Objeto SparkFun (MAX30105 cobre o MAX30102)
  MAX30105 g_sensor;

  // Estado interno simples
  bool g_present   = false;
  bool g_streaming = false;
  bool g_shutdown  = false;

  // Parâmetros ótimos confirmados em teste
  constexpr int      CFG_SAMPLE_RATE_SPS = 100;    // 100 sps
  constexpr int      CFG_PULSE_WIDTH_US  = 411;    // 411 us (máx. resolução)
  constexpr uint8_t  CFG_AVERAGING       = 4;      // FIFO avg = 4
  constexpr uint16_t CFG_ADC_RANGE       = 16384;  // evita saturação
  constexpr uint8_t  CFG_LED_RED         = 0x1F;   // ~6–7 mA (SpO2)
  constexpr uint8_t  CFG_LED_IR          = 0x1F;   // ~6–7 mA (SpO2)
  constexpr uint8_t  CFG_LED_MODE        = 2;      // IR + RED
}

// Aplica o conjunto de parâmetros validados (sempre igual)
// por quê: garante consistência e evita “drift” de configs entre testes
static void apply_config_steady() {
  g_sensor.setLEDMode(CFG_LED_MODE);
  g_sensor.setFIFOAverage(CFG_AVERAGING);
  g_sensor.setSampleRate(CFG_SAMPLE_RATE_SPS);
  g_sensor.setPulseWidth(CFG_PULSE_WIDTH_US);
  g_sensor.setADCRange(CFG_ADC_RANGE);

  g_sensor.setPulseAmplitudeRed(CFG_LED_RED);
  g_sensor.setPulseAmplitudeIR(CFG_LED_IR);
  g_sensor.setPulseAmplitudeGreen(0x00);        // desliga o green

  g_sensor.enableFIFORollover();                // evita travar se o loop atrasar
  g_sensor.clearFIFO();                         // limpa lixo inicial
}

bool max30102_begin() {
  g_present   = false;
  g_streaming = false;
  g_shutdown  = false;

  // por quê: 400 kHz deu melhores curvas AC e resposta da FIFO
  Wire.setClock(400000);

  // Usa I2C_SPEED_FAST quando disponível (mantém compat. com a sua lib)
  if (!g_sensor.begin(Wire, I2C_SPEED_FAST)) {
    return false;
  }

  g_sensor.softReset();
  delay(10);
  g_sensor.setup();              // defaults da SparkFun
  apply_config_steady();

  g_present = true;
  return true;
}

bool max30102_start() {
  if (!g_present)  return false;
  if (g_streaming) return true;

  if (g_shutdown) {              // acorda se estava em shutdown
    g_sensor.wakeUp();
    delay(10);
  }
  apply_config_steady();

  g_streaming = true;
  g_shutdown  = false;
  return true;
}

bool max30102_stop() {
  if (!g_present) return false;
  if (!g_streaming && g_shutdown) return true;

  g_sensor.shutDown();           // economiza energia
  g_streaming = false;
  g_shutdown  = true;
  return true;
}

bool max30102_available() {
  if (!g_present) return false;
  g_sensor.check();              // atualiza ponteiro interno da FIFO
  return g_sensor.available();
}

bool max30102_readRaw(uint32_t* red, uint32_t* ir) {
  if (!red || !ir)          return false;
  if (!g_present)           return false;
  if (!g_sensor.available()) return false;

  // por quê: leitura sincronizada dos dois canais via FIFO
  *red = g_sensor.getFIFORed();
  *ir  = g_sensor.getFIFOIR();
  g_sensor.nextSample();
  return true;
}

bool max30102_isPresent() {
  return g_present;
}
