#ifndef UI_EVENT_BRIDGE_H
#define UI_EVENT_BRIDGE_H

#include "lvgl.h"

#ifdef __cpluscplus
extern "c" {
#endif

/**
 * @brief Inicia uma nova sessão de registro de dados.
 * * Chamado pela UI quando o usuário confirma um nome.
 * - Gera um currentId único baseado no RTC.
 * - Abre o arquivo no SD Card via openDailyIfNeeded.
 * * @param userId O nome do usuário vindo do lv_textarea.
 */
void app_start_logging_session(const char* userId);

/**
 * @brief Para a sessão de registro de dados atual.
 * * Chamado pela UI quando o usuário sai da tela ou clica na área de texto novamente.
 */
void app_stop_logging_session(void);

/**
 * @brief Carrega um preview dos dados do SD Card e exibe em um label.
 * * Chamado pela tela 'ui_Carregar_Dados' ao ser iniciada.
 */
void app_load_data_preview(lv_obj_t* label);

#ifdef __cpluscplus
}
#endif

#endif