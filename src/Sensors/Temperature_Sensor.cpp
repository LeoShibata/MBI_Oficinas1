#include "Temperature_Sensor.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

static bool sensor_ok = false;

void init_temperature_sensor() {
    if(!mlx.begin()) {
        Serial.println("[Sensor] ERROR: MLX90614 not found.");
        Serial.println("[Sensor] Verify connection I2C (SDA = 27, SCL = 22).");
        sensor_ok = false;
    } else {
        Serial.println("[Sensor] MLX90614 successfully started.");
        sensor_ok = true;
    }
}

float read_temperature() {
    if(!sensor_ok) {
        return -999.f;
    }

    float temp = mlx.readObjectTempC();

    // A biblioteca retorna NaN (Not-a-Number) se a leitura falhar
    if(isnan(temp)) {
        Serial.println("[Sensor] Failed to read the temperature (NaN).");
        return -999.f;
    }

    return temp;
}