cmake_minimum_required(VERSION 3.8)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/shell.c)
target_sources(app PRIVATE src/test.c)

target_sources_ifdef(
  CONFIG_BT
  app PRIVATE
  src/bluetooth/blues.c
  src/bluetooth/light.c
  src/bluetooth/health.c
  src/bluetooth/throughput.c
  src/bluetooth/mesh.c
  )

target_sources_ifdef(
  CONFIG_BT_WIFIMGR_SERVICE
  app PRIVATE
  src/bluetooth/wifi_manager_service.c
  )
