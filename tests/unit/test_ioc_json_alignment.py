from __future__ import annotations

import json
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


def _load_json(name: str) -> dict:
    return json.loads((REPO_ROOT / "docs" / "ioc_json" / name).read_text(encoding="utf-8"))


def test_imported_ioc_json_bundle_is_complete() -> None:
    expected = {
        "can_config.json",
        "clock_config.json",
        "dma_config.json",
        "gpio_config.json",
        "i2c_config.json",
        "nvic_config.json",
        "rtc_config.json",
        "rtos_config.json",
        "spi_config.json",
        "summary.json",
        "tim_config.json",
        "usart_config.json",
        "usb_config.json",
    }
    actual = {path.name for path in (REPO_ROOT / "docs" / "ioc_json").glob("*.json")}

    assert actual == expected


def test_board_dts_matches_imported_key_peripherals() -> None:
    board_dts = (
        REPO_ROOT / "boards" / "arm" / "rscf_a_f427iih6" / "rscf_a_f427iih6.dts"
    ).read_text(encoding="utf-8")
    summary = _load_json("summary.json")
    spi_cfg = _load_json("spi_config.json")
    usb_cfg = _load_json("usb_config.json")

    assert summary["mcu_info"]["cpn"] == "STM32F427IIH6"
    assert summary["peripherals"]["spi"] == ["SPI4", "SPI5"]
    assert summary["peripherals"]["tim"] == ["TIM12", "TIM2", "TIM4", "TIM5", "TIM8"]
    assert summary["peripherals"]["usb"] == ["USB_DEVICE", "USB_OTG_FS"]

    assert "&spi4 {" in board_dts
    assert "&spi5 {" in board_dts
    assert "&timers4 {" in board_dts
    assert "&timers5 {" in board_dts
    assert "&timers8 {" in board_dts
    assert "&timers12 {" in board_dts
    assert "tim5_ch1_ph10" in board_dts
    assert "tim8_ch1_pi5" in board_dts
    assert "tim12_ch1_ph6" in board_dts
    assert "&i2c2 {\n  status = \"disabled\";" in board_dts

    assert spi_cfg["instances"]["SPI4"]["pins"]["PE12"] == "SPI4_SCK"
    assert spi_cfg["instances"]["SPI5"]["clock_polarity"] == "SPI_POLARITY_HIGH"
    assert usb_cfg["class_type"] == "CDC"
    assert usb_cfg["device_info"]["pid"] == "2026"


def test_build_uses_ioc_validation_script() -> None:
    app_cmake = (REPO_ROOT / "app" / "CMakeLists.txt").read_text(encoding="utf-8")
    prj_conf = (REPO_ROOT / "app" / "prj.conf").read_text(encoding="utf-8")

    assert "rscf_ioc_validate" in app_cmake
    assert "validate_board_from_ioc.py" in app_cmake
    assert "CONFIG_I2C=n" in prj_conf
