#ifndef GSR_MODULE_H
#define GSR_MODULE_H

/**
 * @brief Inicializa o GSR (configura ADC).
 * Por quê: fixa atenuação/precisão compatíveis com 3V3 e R=220k.
 */
void gsr_init();

/**
 * @brief Lê nível de estresse (0..100).
 * @return 0..100 (quanto maior, maior ativação); -999.f em erro.
 * Por quê: entrega um único número pronto para SD/UI.
 */
float gsr_read_stress();

float gsr_read_microSiemens();  // NOVO: leitura instantânea em µS (ou <0 se inválida)

#endif
