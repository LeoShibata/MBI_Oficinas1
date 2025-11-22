#ifndef GSR_MODULE_H
#define GSR_MODULE_H

/**
 * @brief Inicializa o GSR (ADC e parâmetros do divisor).
 * Por quê: fixa atenuação/precisão do ADC compatível com 3V3 e R=220k.
 */
void gsr_init();

/**
 * @brief Lê o nível de estresse (0..100).
 * @return Valor 0..100 (quanto maior, maior ativação/condutância); -999.f em erro.
 * Por quê: entrega um único número pronto para o SD/UI, sem expor detalhes elétricos.
 */
float gsr_read_stress();

#endif
