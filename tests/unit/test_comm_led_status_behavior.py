from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


def test_comm_led_status_behavior_is_wired() -> None:
    comm_template = (
        REPO_ROOT
        / "mcu_comm"
        / "protocol_generator"
        / "templates"
        / "zephyr"
        / "comm_manager_c.j2"
    ).read_text(encoding="utf-8")
    comm_service = (
        REPO_ROOT / "modules" / "rscf" / "services" / "rscf_comm_service.c"
    ).read_text(encoding="utf-8")
    led_header = (
        REPO_ROOT / "drivers" / "rscf" / "rscf_led_status.h"
    ).read_text(encoding="utf-8")
    led_driver = (
        REPO_ROOT / "drivers" / "rscf" / "rscf_led_status.c"
    ).read_text(encoding="utf-8")

    assert "Comm_OnValidRxFrame" in comm_template
    assert "Comm_OnValidRxFrame(pid);" in comm_template

    assert "RSCF_COMM_LED_FAST_HALF_MS 100U" in comm_service
    assert "RSCF_COMM_LED_SLOW_HALF_MS 500U" in comm_service
    assert "RSCFLedStatusSetBlinkHalfPeriod(RSCF_LED_STATUS_COMM" in comm_service

    assert "RSCFLedStatusSetBlinkHalfPeriod" in led_header
    assert "s_blink_half_period_ms" in led_driver


def test_comm_transport_adapter_and_compat_hooks_are_explicit() -> None:
    comm_template = (
        REPO_ROOT
        / "mcu_comm"
        / "protocol_generator"
        / "templates"
        / "zephyr"
        / "comm_manager_c.j2"
    ).read_text(encoding="utf-8")
    generated_comm = (
        REPO_ROOT / "generated" / "mcu_comm" / "MCU_ONE" / "core" / "comm_manager.c"
    ).read_text(encoding="utf-8")
    generated_robot_h = (
        REPO_ROOT / "generated" / "mcu_comm" / "MCU_ONE" / "app" / "robot_interface.h"
    ).read_text(encoding="utf-8")
    generated_robot_c = (
        REPO_ROOT / "generated" / "mcu_comm" / "MCU_ONE" / "app" / "robot_interface.c"
    ).read_text(encoding="utf-8")
    comm_service = (
        REPO_ROOT / "modules" / "rscf" / "services" / "rscf_comm_service.c"
    ).read_text(encoding="utf-8")
    app_cmake = (REPO_ROOT / "app" / "CMakeLists.txt").read_text(encoding="utf-8")

    assert "Comm_ResolveTransportOrDefault" in comm_template
    assert "Comm_OnTransportResolved" in comm_template
    assert "Comm_CompatMapEndpoint" in comm_template

    assert "Comm_ResolveTransportOrDefault" in generated_comm
    assert "Comm_OnTransportResolved" in generated_comm
    assert "Comm_CompatMapEndpoint" in generated_comm

    assert "RobotInterfaceResolveTransportOverride" in generated_robot_h
    assert "RobotInterfaceOnTransportReady" in generated_robot_h
    assert "RobotInterfaceFillCompatSystemStatus" in generated_robot_h
    assert "RobotInterfaceOnCompatSystemCmd" in generated_robot_h

    assert "RobotInterfaceResolveTransportOverride" in generated_robot_c
    assert "RobotInterfaceOnTransportReady" in generated_robot_c
    assert "RobotInterfaceFillCompatSystemStatus" in generated_robot_c
    assert "RobotInterfaceOnCompatSystemCmd" in generated_robot_c
    assert "RobotInterfaceCompatSystemCmdCallback" in generated_robot_c

    assert "RSCFCommServiceSelectTransport" in comm_service
    assert "RSCFCommServiceSelectAdapterRole" in comm_service
    assert "RSCFLinkUsbAdapterGetTransport" in comm_service
    assert "RSCFLinkUartAdapterGetTransport" in comm_service

    assert "../drivers/rscf/rscf_link_usb.c" in app_cmake
    assert "../drivers/rscf/rscf_link_uart.c" in app_cmake
    assert "../drivers/rscf/rscf_link_can.c" in app_cmake
    assert "../drivers/rscf/rscf_link_spi.c" in app_cmake
