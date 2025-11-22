#include "Temperature_Sensor.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>

static Adafruit_MLX90614 mlx;
static bool sensor_ok = false;

// Config local para o MLX (isolada dos demais módulos)
static constexpr uint8_t  I2C_SDA = 27;
static constexpr uint8_t  I2C_SCL = 22;
static constexpr uint32_t I2C_HZ  = 100000;  // por quê: MLX é SMBus 100 kHz

static inline bool plausivel(float t) {
  // por quê: faixa típica do MLX para descartar leituras quebradas
  return !isnan(t) && t > -40.f && t < 125.f;
}

void init_temperature_sensor() {
  // por quê: força I2C local a 100 kHz e timeout maior
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(I2C_HZ);
  Wire.setTimeOut(2500);
  delay(50);

  if (!mlx.begin(0x5A, &Wire)) {
    Serial.println("[Sensor] ERROR: MLX90614 not found. Check SDA=27, SCL=22.");
    sensor_ok = false;
    return;
  }

  // Leitura de saneamento (3 tentativas)
  for (int i = 0; i < 3; ++i) {
    float t = mlx.readObjectTempC();
    if (plausivel(t)) { sensor_ok = true; Serial.println("[Sensor] MLX90614 OK."); return; }
    delay(10);
  }
  Serial.println("[Sensor] MLX90614 inicializou, mas leitura falhou.");
  sensor_ok = false;
}

float read_temperature() {
  if (!sensor_ok) return -999.f;

  for (int i = 0; i < 3; ++i) {
    float temp = mlx.readObjectTempC();
    if (plausivel(temp)) return temp;
    delay(5);
  }
  Serial.println("[Sensor] Falha ao ler temperatura (NaN/fora de faixa).");
  return -999.f;
}
