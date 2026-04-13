cmake_minimum_required(VERSION 3.20.0)

list(APPEND BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)
list(APPEND DTS_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)
set(RSCF_SEGGER_MODULE_DIR ${CMAKE_CURRENT_LIST_DIR}/../modules/debug/segger)
set(RSCF_MICROROS_MODULE_DIR ${CMAKE_CURRENT_LIST_DIR}/../modules/lib/libmicroros)

if(EXISTS ${RSCF_SEGGER_MODULE_DIR}/zephyr/module.yml)
  list(APPEND ZEPHYR_EXTRA_MODULES ${RSCF_SEGGER_MODULE_DIR})
endif()

if(EXISTS ${RSCF_MICROROS_MODULE_DIR}/zephyr/module.yml)
  list(APPEND ZEPHYR_EXTRA_MODULES ${RSCF_MICROROS_MODULE_DIR})
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
find_package(Python3 REQUIRED COMPONENTS Interpreter)
project(rscf_a_zephyr)
get_filename_component(RSCF_WEST_TOPDIR ${ZEPHYR_BASE} DIRECTORY)

set(RSCF_CMSIS_DSP_DIR ${CMAKE_CURRENT_LIST_DIR}/../third_party/cmsis_dsp)
set(RSCF_CMSIS_DSP_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/../third_party/cmsis_dsp/Include)
set(RSCF_CMSIS_DSP_PRIVATE_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/../third_party/cmsis_dsp/PrivateInclude)

set(RSCF_MCU_DEVICE "MCU_ONE" CACHE STRING "mcu_comm target MCU device")
set(RSCF_MCU_COMM_OUTPUT_DIR ${CMAKE_BINARY_DIR}/generated/mcu_comm/${RSCF_MCU_DEVICE})
set(RSCF_MCU_COMM_APP_DIR ${RSCF_MCU_COMM_OUTPUT_DIR}/app)
set(RSCF_MCU_COMM_CORE_DIR ${RSCF_MCU_COMM_OUTPUT_DIR}/core)
set(RSCF_MCU_COMM_TRANSPORT_DIR ${RSCF_MCU_COMM_OUTPUT_DIR}/transports)
set(RSCF_MCU_COMM_SYNC_DIR ${CMAKE_CURRENT_LIST_DIR}/../generated/mcu_comm)
file(GLOB RSCF_MCU_COMM_ZEPHYR_TEMPLATES
  ${CMAKE_CURRENT_LIST_DIR}/../mcu_comm/protocol_generator/templates/zephyr/*.j2
)

set(RSCF_MCU_COMM_GENERATED_HEADERS
  ${RSCF_MCU_COMM_APP_DIR}/data_def.h
  ${RSCF_MCU_COMM_APP_DIR}/robot_interface.h
  ${RSCF_MCU_COMM_CORE_DIR}/protocol.h
  ${RSCF_MCU_COMM_CORE_DIR}/comm_manager.h
  ${RSCF_MCU_COMM_TRANSPORT_DIR}/transport_interface.h
  ${RSCF_MCU_COMM_TRANSPORT_DIR}/transport_uart.h
  ${RSCF_MCU_COMM_TRANSPORT_DIR}/transport_can.h
  ${RSCF_MCU_COMM_TRANSPORT_DIR}/transport_usb.h
)

set(RSCF_MCU_COMM_GENERATED_SOURCES
  ${RSCF_MCU_COMM_APP_DIR}/data_def.c
  ${RSCF_MCU_COMM_APP_DIR}/robot_interface.c
  ${RSCF_MCU_COMM_CORE_DIR}/protocol.c
  ${RSCF_MCU_COMM_CORE_DIR}/comm_manager.c
  ${RSCF_MCU_COMM_TRANSPORT_DIR}/transport_uart.c
  ${RSCF_MCU_COMM_TRANSPORT_DIR}/transport_can.c
  ${RSCF_MCU_COMM_TRANSPORT_DIR}/transport_usb.c
)

add_custom_command(
  OUTPUT ${RSCF_MCU_COMM_GENERATED_HEADERS} ${RSCF_MCU_COMM_GENERATED_SOURCES}
  COMMAND ${Python3_EXECUTABLE}
          ${CMAKE_CURRENT_LIST_DIR}/../scripts/codegen/rscf_mcu_comm_codegen.py
          --device ${RSCF_MCU_DEVICE}
          --output-dir ${RSCF_MCU_COMM_OUTPUT_DIR}
  DEPENDS
          ${CMAKE_CURRENT_LIST_DIR}/../scripts/codegen/rscf_mcu_comm_codegen.py
          ${CMAKE_CURRENT_LIST_DIR}/../mcu_comm/protocol_generator/generator.py
          ${CMAKE_CURRENT_LIST_DIR}/../mcu_comm/protocol_generator/protocol_definition.json
          ${CMAKE_CURRENT_LIST_DIR}/../mcu_comm/protocol_generator/system_status_schema.yaml
          ${CMAKE_CURRENT_LIST_DIR}/../mcu_comm/protocol_generator/system_cmd_schema.yaml
          ${CMAKE_CURRENT_LIST_DIR}/../mcu_comm/protocol_generator/control_plane_enums.yaml
          ${RSCF_MCU_COMM_ZEPHYR_TEMPLATES}
  VERBATIM
)

add_custom_target(rscf_mcu_comm_codegen DEPENDS
  ${RSCF_MCU_COMM_GENERATED_HEADERS}
  ${RSCF_MCU_COMM_GENERATED_SOURCES}
)

add_custom_target(rscf_mcu_comm_sync
  COMMAND ${Python3_EXECUTABLE}
          ${CMAKE_CURRENT_LIST_DIR}/../scripts/codegen/rscf_mcu_comm_codegen.py
          --device ${RSCF_MCU_DEVICE}
          --output-dir ${RSCF_MCU_COMM_OUTPUT_DIR}
          --sync-dir ${RSCF_MCU_COMM_SYNC_DIR}
          --sync
  DEPENDS rscf_mcu_comm_codegen
  VERBATIM
)

add_custom_target(rscf_ioc_validate
  COMMAND ${Python3_EXECUTABLE}
          ${CMAKE_CURRENT_LIST_DIR}/../scripts/ioc/validate_board_from_ioc.py
          --repo-root ${CMAKE_CURRENT_LIST_DIR}/..
  VERBATIM
)

zephyr_include_directories(${ZEPHYR_BASE}/modules/cmsis_6)
zephyr_include_directories(${RSCF_WEST_TOPDIR}/third_party/hal/cmsis/CMSIS/Core/Include)
zephyr_compile_definitions(SHPR=SHP)
target_compile_definitions(app PRIVATE ARM_MATH_CM4 ARM_MATH_MATRIX_CHECK ARM_MATH_ROUNDING)

target_include_directories(app PRIVATE
  ${RSCF_CMSIS_DSP_INCLUDE_DIR}
  ${RSCF_CMSIS_DSP_PRIVATE_INCLUDE_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/../include
  ${CMAKE_CURRENT_LIST_DIR}/../drivers/rscf
  ${CMAKE_CURRENT_LIST_DIR}/../drivers/bsp
  ${CMAKE_CURRENT_LIST_DIR}/../modules/rscf/services
  ${CMAKE_CURRENT_LIST_DIR}/../modules/rscf/devices
  ${RSCF_MCU_COMM_APP_DIR}
  ${RSCF_MCU_COMM_CORE_DIR}
  ${RSCF_MCU_COMM_TRANSPORT_DIR}
)

target_sources(app PRIVATE
  src/main.c
  ../drivers/bsp/bsp_usart.c
  ../drivers/bsp/bsp_log.c
  ../drivers/bsp/bsp_dwt.c
  ../drivers/bsp/bsp_can.c
  ../drivers/bsp/bsp_spi.c
  ../drivers/bsp/bsp_pwm.c
  ../drivers/bsp/bsp_tim.c
  ../drivers/rscf/rscf_boot.c
  ../drivers/rscf/rscf_led_status.c
  ../drivers/rscf/rscf_link_usb.c
  ../drivers/rscf/rscf_link_uart.c
  ../drivers/rscf/rscf_link_can.c
  ../drivers/rscf/rscf_link_spi.c
  ../drivers/rscf/rscf_ext_uart_mux.c
  ../drivers/rscf/rscf_buzzer.c
  ../drivers/rscf/rscf_power_switch.c
  ../modules/algorithm/coordinate_system.c
  ../modules/algorithm/crc8.c
  ../modules/algorithm/crc16.c
  ../modules/algorithm/linear_algebra.c
  ../modules/algorithm/Kalman_filter.c
  ../modules/algorithm/PID.c
  ../modules/algorithm/user_lib.c
  ../modules/compat/unit_convert.c
  ../modules/compat/controller.c
  ../modules/compat/daemon.c
  ../modules/compat/led.c
  ../modules/compat/buzzer.c
  ../modules/compat/hc05.c
  ../modules/compat/seasky_protocol.c
  ../modules/compat/master_process.c
  ../modules/compat/mhall.c
  ../modules/compat/ad7190.c
  ../modules/compat/oled.c
  ../modules/compat/remote_control.c
  ../modules/compat/robot.c
  ../modules/compat/robot_cmd.c
  ../modules/compat/motor_task.c
  ../third_party/cmsis_dsp/Source/CommonTables/arm_common_tables.c
  ../third_party/cmsis_dsp/Source/FastMathFunctions/arm_sin_f32.c
  ../third_party/cmsis_dsp/Source/FastMathFunctions/arm_cos_f32.c
  ../third_party/cmsis_dsp/Source/FastMathFunctions/arm_atan2_f32.c
  ../third_party/cmsis_dsp/Source/MatrixFunctions/arm_mat_init_f32.c
  ../third_party/cmsis_dsp/Source/MatrixFunctions/arm_mat_add_f32.c
  ../third_party/cmsis_dsp/Source/MatrixFunctions/arm_mat_sub_f32.c
  ../third_party/cmsis_dsp/Source/MatrixFunctions/arm_mat_mult_f32.c
  ../third_party/cmsis_dsp/Source/MatrixFunctions/arm_mat_trans_f32.c
  ../third_party/cmsis_dsp/Source/MatrixFunctions/arm_mat_inverse_f32.c
  ../modules/motor/dji_motor.c
  ../modules/motor/dmmotor.c
  ../modules/motor/servo_motor.c
  ../modules/rscf/core/rscf_app_profile.c
  ../modules/rscf/services/rscf_event_bus.c
  ../modules/rscf/services/rscf_link_service.c
  ../modules/rscf/services/rscf_service_router.c
  ../modules/rscf/services/rscf_action_service.c
  ../modules/rscf/services/rscf_daemon_service.c
  ../modules/rscf/services/rscf_health_service.c
  ../modules/rscf/services/rscf_comm_service.c
  ../modules/rscf/services/rscf_chassis_service.c
  ../modules/rscf/services/rscf_robot_service.c
  ../modules/rscf/services/rscf_ros_bridge.c
  ../modules/rscf/services/rscf_motor_service.c
  ../modules/rscf/services/rscf_debug_fault.c
  ../modules/rscf/devices/rscf_dtof2510.c
  ../modules/rscf/devices/rscf_imu_uart.c
  ../modules/rscf/devices/rscf_saber_imu_compat.c
  ${RSCF_MCU_COMM_GENERATED_SOURCES}
)

add_dependencies(app rscf_mcu_comm_codegen)
add_dependencies(app rscf_ioc_validate)

if(CONFIG_RSCF_MICROROS_SERVICE)
  target_sources(app PRIVATE
    ../modules/rscf/services/rscf_microros_service.c
  )
endif()
