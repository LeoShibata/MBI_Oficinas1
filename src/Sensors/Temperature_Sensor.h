#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

/**
 * @brief Inicializa o sensor de temperatura MXL90614.
 */
void init_temperature_sensor();

/**
 * @brief Lê a temperatura do objeto (paciente) do sensor.
 */
float read_temperature();

#endif