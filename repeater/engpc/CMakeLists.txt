zephyr_include_directories(.)

zephyr_sources_ifdef(
    CONFIG_ENGPC
    engpc_main.c
    eng_diag.c
    eng_cmd4linuxhdlr.c
    wifi_eut_sprd.c
    eng_sscanf.c
    )
