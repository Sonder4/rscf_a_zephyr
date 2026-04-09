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
        "modules/rscf/devices/rscf_dtof2510.c",
        "modules/rscf/devices/rscf_dtof2510.h",
        "modules/rscf/devices/rscf_imu_uart.c",
        "modules/rscf/devices/rscf_imu_uart.h",
        "modules/rscf/services/rscf_comm_service.c",
        "modules/rscf/services/rscf_comm_service.h",
        "modules/rscf/services/rscf_daemon_service.c",
        "modules/rscf/services/rscf_debug_fault.c",
        "include/rscf/rscf_daemon_service.h",
        "include/rscf/rscf_debug_fault.h",
        "dts/bindings/rscf/ncurc,rscf-ext-uart-mux.yaml",
        "dts/bindings/rscf/ncurc,rscf-dtof2510-group.yaml",
        "dts/bindings/rscf/ncurc,rscf-buzzer.yaml",
    ]

    for relpath in expected:
        assert (REPO_ROOT / relpath).is_file(), relpath


def test_app_cmake_references_new_modules() -> None:
    app_cmake = (REPO_ROOT / "app" / "CMakeLists.txt").read_text(encoding="utf-8")

    assert "rscf_led_status.c" in app_cmake
    assert "rscf_ext_uart_mux.c" in app_cmake
    assert "rscf_buzzer.c" in app_cmake
    assert "rscf_comm_service.c" in app_cmake
    assert "rscf_daemon_service.c" in app_cmake
    assert "rscf_debug_fault.c" in app_cmake
    assert "rscf_dtof2510.c" in app_cmake
    assert "rscf_imu_uart.c" in app_cmake


def test_board_dts_declares_custom_nodes() -> None:
    board_dts = (REPO_ROOT / "boards" / "arm" / "rscf_a_f427iih6" / "rscf_a_f427iih6.dts").read_text(
        encoding="utf-8"
    )

    assert 'compatible = "ncurc,rscf-ext-uart-mux";' in board_dts
    assert 'compatible = "ncurc,rscf-dtof2510-group";' in board_dts
    assert 'compatible = "ncurc,rscf-buzzer";' in board_dts


def test_profile_initializes_new_services() -> None:
    profile_c = (REPO_ROOT / "modules" / "rscf" / "core" / "rscf_app_profile.c").read_text(encoding="utf-8")

    assert "RSCFLedStatusInit" in profile_c
    assert "RSCFCommServiceInit" in profile_c
    assert "RSCFDaemonServiceInit" in profile_c
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
    assert "config RSCF_DEBUG_FAULT" in modules_kconfig
