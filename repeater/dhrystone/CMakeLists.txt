zephyr_include_directories(.)

zephyr_compile_definitions(
    -DNO_PROTOTYPES=1
    -DHZ=100
)

zephyr_compile_options(
    -mfloat-abi=soft
    -Wno-implicit-int
    -Wno-implicit-function-declaration
    -Wno-return-type
    -Wno-maybe-uninitialized
)

zephyr_sources_ifdef(
    CONFIG_DHRYSTONE
    dhry_1.c
    dhry_2.c
    dhrystone.c
    )
