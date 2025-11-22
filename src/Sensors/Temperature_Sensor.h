#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

/**
 * @brief Inicializa o MLX90614.
 * Por quê: o MLX é SMBus (100 kHz) e pode falhar em 400 kHz.
 */
void init_temperature_sensor();

/**
 * @brief Lê a temperatura do objeto (°C).
 * @return Temperatura válida em °C, ou -999.f em erro.
 * Por quê: -999.f é sentinela simples para UI/CSV.
 */
float read_temperature();

#endif
