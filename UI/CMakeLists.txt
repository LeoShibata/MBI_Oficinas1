SET(SOURCES ui_Tela_Inicial.c
    ui_Tela_de_Modo.c
    ui_Modo_Consulta.c
    ui_Modo_Registro.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c)

add_library(ui ${SOURCES})
