zephyr_include_directories(.)

zephyr_sources_ifdef(
    CONFIG_IWNPI
    cmd2355.c
    wlnpi.c
    wifi_npi.c
    )
