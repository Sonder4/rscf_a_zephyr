cmake_minimum_required(VERSION 3.20.0)

list(APPEND BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)
list(APPEND DTS_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(rscf_a_zephyr)

target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/../include
)

target_sources(app PRIVATE
  src/main.c
  ../drivers/rscf/rscf_boot.c
  ../modules/rscf/core/rscf_app_profile.c
  ../modules/rscf/services/rscf_event_bus.c
  ../modules/rscf/services/rscf_health_service.c
)
