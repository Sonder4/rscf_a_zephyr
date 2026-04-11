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
