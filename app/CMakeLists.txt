cmake_minimum_required(VERSION 3.20.0)

list(APPEND BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)
list(APPEND DTS_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(rscf_a_zephyr)

zephyr_include_directories(${ZEPHYR_BASE}/modules/cmsis_6)
zephyr_include_directories(${CMAKE_CURRENT_LIST_DIR}/../.workspace/third_party/hal/cmsis/CMSIS/Core/Include)
zephyr_compile_definitions(SHPR=SHP)

target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/../include
  ${CMAKE_CURRENT_LIST_DIR}/../drivers/rscf
  ${CMAKE_CURRENT_LIST_DIR}/../modules/rscf/services
  ${CMAKE_CURRENT_LIST_DIR}/../modules/rscf/devices
)

target_sources(app PRIVATE
  src/main.c
  ../drivers/rscf/rscf_boot.c
  ../drivers/rscf/rscf_led_status.c
  ../drivers/rscf/rscf_ext_uart_mux.c
  ../drivers/rscf/rscf_buzzer.c
  ../modules/rscf/core/rscf_app_profile.c
  ../modules/rscf/services/rscf_event_bus.c
  ../modules/rscf/services/rscf_daemon_service.c
  ../modules/rscf/services/rscf_health_service.c
  ../modules/rscf/services/rscf_comm_service.c
  ../modules/rscf/services/rscf_debug_fault.c
  ../modules/rscf/devices/rscf_dtof2510.c
  ../modules/rscf/devices/rscf_imu_uart.c
)
