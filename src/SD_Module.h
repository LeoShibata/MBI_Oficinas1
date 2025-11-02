#ifndef SD_MODULE_H
#define SD_MODULE_H

#include <Arduino.h>

void CSV_init();
bool openDailyIfNeeded();
bool CSV_rotateIfNeeded();  // stub no MVP
bool CSV_appendRow(const char* userId,
                   const char* sessionId,
                   const char* sensor,
                   double valor,
                   const char* unidade);
void CSV_close();

const char* CSV_get_current_path(void);

/**
 * @brief Constrói os caminhos de diretório e arquivo com base na data/hora atual.
 * @param dir_year Buffer para armazenar o path do diretório (ex: /MBI/CSV/2025)
 * @param dir_sz Tamanho do buffer dir_year
 * @param file_path Buffer para armazenar o path do arquivo (ex: /MBI/CSV/2025/MBI_20250101.csv)
 * @param path_sz Tamanho do buffer file_path
 * @param today_out Ponteiro para int para armazenar a data (AAAAMMDD)
 * @param rtc_ok_out Ponteiro para bool para indicar se o RTC está válido
 */
 void build_path_safe(char* dir_year, size_t dir_sz,
                     char* file_path, size_t path_sz, 
                     int* today_out, bool* rtc_ok_out);  

#endif