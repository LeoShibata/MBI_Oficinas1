#include "Temperature_Sensor.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>

static Adafruit_MLX90614 mlx;     // escopo do TU p/ evitar colisão
static bool sensor_ok = false;

// Pinos/clock I²C dedicados para este módulo (isolado dos demais).
static constexpr uint8_t  I2C_SDA = 27;
static constexpr uint8_t  I2C_SCL = 22;
static constexpr uint32_t I2C_HZ  = 100000;   // por quê: MLX é SMBus 100 kHz

// Faixa plausível do MLX para sanity check (datasheet)
static inline bool temp_plausivel(float t) {
  return !isnan(t) && t > -40.f && t < 125.f; // por quê: filtra leituras quebradas
}

void init_temperature_sensor() {
  // Força configuração I²C local para o MLX.
  // por quê: reduzir NaN/timeout quando outros módulos usam 400 kHz.
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(I2C_HZ);
  Wire.setTimeOut(2500); // por quê: MLX pode fazer clock-stretch

  delay(50); // por quê: margem breve de estabilização após power-on

  if (!mlx.begin(0x5A, &Wire)) {
    Serial.println("[Sensor] ERROR: MLX90614 not found.");
    Serial.println("[Sensor] Verifique I2C (SDA=27, SCL=22).");
    sensor_ok = false;
    return;
  }

  // Sanity read com pequenas tentativas, reduz falsos NaN na primeira leitura
  for (int i = 0; i < 3; ++i) {
    float t = mlx.readObjectTempC();
    if (temp_plausivel(t)) {
      sensor_ok = true;
      Serial.println("[Sensor] MLX90614 inicializado.");
      return;
    }
    delay(10);
  }

  Serial.println("[Sensor] Falha de leitura inicial do MLX90614.");
  sensor_ok = false;
}

float read_temperature() {
  if (!sensor_ok) return -999.f;

  // Leitura simples com até 3 tentativas (transientes do barramento)
  for (int i = 0; i < 3; ++i) {
    float temp = mlx.readObjectTempC();
    if (temp_plausivel(temp)) return temp;
    delay(5); // por quê: aguarda breve antes de nova tentativa
  }

  Serial.println("[Sensor] Falha ao ler temperatura (NaN/fora de faixa).");
  return -999.f;
}
