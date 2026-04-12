from __future__ import annotations

from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


def test_next_wave_driver_and_module_files_exist() -> None:
    expected = [
        "drivers/rscf/rscf_ext_uart_mux.c",
        "drivers/rscf/rscf_ext_uart_mux.h",
        "drivers/rscf/rscf_led_status.c",
        "drivers/rscf/rscf_led_status.h",
        "drivers/rscf/rscf_buzzer.c",
        "drivers/rscf/rscf_buzzer.h",
        "drivers/rscf/rscf_power_switch.c",
        "drivers/rscf/rscf_power_switch.h",
        "modules/rscf/devices/rscf_dtof2510.c",
        "modules/rscf/devices/rscf_dtof2510.h",
        "modules/rscf/devices/rscf_imu_uart.c",
        "modules/rscf/devices/rscf_imu_uart.h",
        "modules/motor/dji_motor.c",
        "modules/motor/dmmotor.c",
        "modules/motor/servo_motor.c",
        "include/dji_motor.h",
        "include/dmmotor.h",
        "include/servo_motor.h",
        "include/gpio.h",
        "include/spi.h",
        "include/tim.h",
        "include/led.h",
        "include/buzzer.h",
        "include/bsp_spi.h",
        "include/bsp_pwm.h",
        "include/bsp_tim.h",
        "modules/rscf/services/rscf_comm_service.c",
        "modules/rscf/services/rscf_comm_service.h",
        "modules/rscf/services/rscf_chassis_service.c",
        "include/rscf/rscf_chassis_service.h",
        "modules/rscf/services/rscf_robot_service.c",
        "include/rscf/rscf_robot_service.h",
        "modules/rscf/services/rscf_daemon_service.c",
        "modules/rscf/services/rscf_debug_fault.c",
        "include/rscf/rscf_daemon_service.h",
        "include/rscf/rscf_debug_fault.h",
        "modules/compat/hc05.c",
        "modules/compat/seasky_protocol.c",
        "modules/compat/master_process.c",
        "modules/compat/mhall.c",
        "modules/compat/ad7190.c",
        "modules/compat/oled.c",
        "modules/compat/remote_control.c",
        "modules/compat/robot.c",
        "modules/compat/robot_cmd.c",
        "drivers/bsp/bsp_spi.c",
        "drivers/bsp/bsp_pwm.c",
        "drivers/bsp/bsp_tim.c",
        "include/HC05.h",
        "include/seasky_protocol.h",
        "include/master_process.h",
        "include/mhall.h",
        "include/AD7190.h",
        "include/oled.h",
        "include/remote_control.h",
        "include/robot.h",
        "include/robot_task.h",
        "include/robot_cmd.h",
        "docs/ioc_json/summary.json",
        "docs/ioc_json/gpio_config.json",
        "docs/ioc_json/spi_config.json",
        "docs/ioc_json/tim_config.json",
        "docs/ioc_json/usart_config.json",
        "docs/ioc_json/usb_config.json",
        "scripts/ioc/validate_board_from_ioc.py",
        "dts/bindings/rscf/ncurc,rscf-ext-uart-mux.yaml",
        "dts/bindings/rscf/ncurc,rscf-dtof2510-group.yaml",
        "dts/bindings/rscf/ncurc,rscf-buzzer.yaml",
        "dts/bindings/rscf/ncurc,rscf-power-switch-bank.yaml",
        "dts/bindings/rscf/ncurc,rscf-servo-pwm.yaml",
    ]

    for relpath in expected:
        assert (REPO_ROOT / relpath).is_file(), relpath


def test_app_cmake_references_new_modules() -> None:
    app_cmake = (REPO_ROOT / "app" / "CMakeLists.txt").read_text(encoding="utf-8")

    assert "rscf_led_status.c" in app_cmake
    assert "rscf_ext_uart_mux.c" in app_cmake
    assert "rscf_buzzer.c" in app_cmake
    assert "rscf_power_switch.c" in app_cmake
    assert "dji_motor.c" in app_cmake
    assert "dmmotor.c" in app_cmake
    assert "servo_motor.c" in app_cmake
    assert "rscf_comm_service.c" in app_cmake
    assert "rscf_chassis_service.c" in app_cmake
    assert "rscf_robot_service.c" in app_cmake
    assert "rscf_daemon_service.c" in app_cmake
    assert "rscf_debug_fault.c" in app_cmake
    assert "rscf_dtof2510.c" in app_cmake
    assert "rscf_imu_uart.c" in app_cmake
    assert "bsp_spi.c" in app_cmake
    assert "bsp_pwm.c" in app_cmake
    assert "bsp_tim.c" in app_cmake
    assert "hc05.c" in app_cmake
    assert "seasky_protocol.c" in app_cmake
    assert "master_process.c" in app_cmake
    assert "mhall.c" in app_cmake
    assert "ad7190.c" in app_cmake
    assert "oled.c" in app_cmake
    assert "remote_control.c" in app_cmake
    assert "robot.c" in app_cmake
    assert "robot_cmd.c" in app_cmake


def test_board_dts_declares_custom_nodes() -> None:
    board_dts = (REPO_ROOT / "boards" / "arm" / "rscf_a_f427iih6" / "rscf_a_f427iih6.dts").read_text(
        encoding="utf-8"
    )

    assert 'compatible = "ncurc,rscf-ext-uart-mux";' in board_dts
    assert 'compatible = "ncurc,rscf-dtof2510-group";' in board_dts
    assert 'compatible = "ncurc,rscf-buzzer";' in board_dts
    assert 'compatible = "ncurc,rscf-power-switch-bank";' in board_dts
    assert 'compatible = "ncurc,rscf-servo-pwm";' in board_dts
    assert "&timers8 {" in board_dts
    assert "tim5_ch1_ph10" in board_dts
    assert "&i2c2 {\n  status = \"disabled\";" in board_dts


def test_profile_initializes_new_services() -> None:
    profile_c = (REPO_ROOT / "modules" / "rscf" / "core" / "rscf_app_profile.c").read_text(encoding="utf-8")

    assert "RSCFLedStatusInit" in profile_c
    assert "RSCFPowerSwitchInit" in profile_c
    assert "RSCFCommServiceInit" in profile_c
    assert "RSCFChassisServiceInit" in profile_c
    assert "RSCFRobotServiceInit" in profile_c
    assert "RSCFDaemonServiceInit" in profile_c
    assert "RSCFMotorServiceInit" in profile_c
    assert "RSCFDebugFaultInit" in profile_c
    assert "RSCFImuUartInit" in profile_c
    assert "RSCFDtof2510Init" in profile_c


def test_event_bus_and_daemon_interfaces_are_exposed() -> None:
    event_bus_h = (REPO_ROOT / "include" / "rscf" / "rscf_event_bus.h").read_text(encoding="utf-8")
    daemon_h = (REPO_ROOT / "include" / "rscf" / "rscf_daemon_service.h").read_text(encoding="utf-8")
    modules_kconfig = (REPO_ROOT / "modules" / "rscf" / "Kconfig").read_text(encoding="utf-8")

    assert "RSCFEventBusPublisherRegister" in event_bus_h
    assert "RSCFEventBusSubscriberRegister" in event_bus_h
    assert "RSCFDaemonRegister" in daemon_h
    assert "config RSCF_DAEMON_SERVICE" in modules_kconfig
    assert "config RSCF_CHASSIS_SERVICE" in modules_kconfig
    assert "config RSCF_ROBOT_SERVICE" in modules_kconfig
    assert "config RSCF_DEBUG_FAULT" in modules_kconfig
