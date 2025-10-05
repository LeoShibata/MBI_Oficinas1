SET(SOURCES ui_Tela_Inicial.c
    ui_Selecao_de_Modo.c
    ui_Modo_Consulta.c
    ui_Modo_Registro.c
    ui_Novo_Registro.c
    ui_Carregar_Dados.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_img_mbi_logo_png.c)

add_library(ui ${SOURCES})
