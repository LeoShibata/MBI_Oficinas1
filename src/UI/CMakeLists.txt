SET(SOURCES ui_comp_cardbase2.c
    ui_comp_combarrastatus.c
    ui_comp_termometro.c
    ui_comp.c
    ui_Tela_Inicial.c
    ui_Selecao_de_Modo.c
    ui_Modo_Registro.c
    ui_Novo_Registro.c
    ui_Dashboard.c
    ui_Tela_Instrumento.c
    ui_Tela_Termometro.c
    ui_Tela_Oximetro.c
    ui_GSR.c
    ui_ECG.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_img_mbi_logo_png.c
    ui_img_586855341.c
    ui_img_939456327.c
    ui_img_1725504982.c
    ui_img_1561148902.c
    ui_img_587847134.c
    ui_img_1635948304.c
    ui_img_652512494.c
    ui_img_1680349653.c
    ui_img_134478313.c
    ui_img_gsr_png.c
    ui_img_177543304.c
    ui_img_96564182.c
    ui_img_141844619.c)

add_library(ui ${SOURCES})
