zephyr_include_directories(.)

zephyr_include_directories($ENV{ZEPHYR_BASE}/ext/hal/unisoc/)

zephyr_sources_ifdef(
  CONFIG_BT_TEST
  bt_eng.c
  bt_npi.c
  bt_utlis.c
  bqb.c
  )
