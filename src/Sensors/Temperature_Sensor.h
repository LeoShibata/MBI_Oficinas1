#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

/**
 * @brief Inicializa o sensor MLX90614.
 * Por quê: usa I2C a 100 kHz e timeout maior para evitar NaN.
 */
void init_temperature_sensor();

/**
 * @brief Lê temperatura do objeto em °C.
 * @return Temperatura válida ou -999.f em erro.
 */
float read_temperature();

#endif
