zephyr_include_directories(.)

zephyr_sources_ifdef(
  CONFGI_BT_TEST
  bt_eng.c
  bt_npi.c
  bt_sipc.c
  bt_utlis.c
  bqb.c
  )
