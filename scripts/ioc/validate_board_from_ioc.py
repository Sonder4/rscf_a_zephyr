#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
from pathlib import Path


def load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def require(text: str, needle: str, message: str) -> None:
    if needle not in text:
        raise SystemExit(message)


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate Zephyr board config against imported ioc_json")
    parser.add_argument("--repo-root", type=Path, required=True)
    args = parser.parse_args()

    repo_root = args.repo_root.resolve()
    ioc_dir = repo_root / "docs" / "ioc_json"
    board_dts = (repo_root / "boards" / "arm" / "rscf_a_f427iih6" / "rscf_a_f427iih6.dts").read_text(
        encoding="utf-8"
    )
    app_cmake = (repo_root / "app" / "CMakeLists.txt").read_text(encoding="utf-8")
    prj_conf = (repo_root / "app" / "prj.conf").read_text(encoding="utf-8")

    summary = load_json(ioc_dir / "summary.json")
    gpio_cfg = load_json(ioc_dir / "gpio_config.json")
    spi_cfg = load_json(ioc_dir / "spi_config.json")
    usb_cfg = load_json(ioc_dir / "usb_config.json")

    for name in summary["peripherals"]["usart"]:
        node_name = name.lower()
        require(board_dts, f"&{node_name} {{", f"missing UART node {name} in board dts")

    for name in summary["peripherals"]["can"]:
        node_name = name.lower()
        require(board_dts, f"&{node_name} {{", f"missing CAN node {name} in board dts")

    for name in summary["peripherals"]["spi"]:
        node_name = name.lower()
        require(board_dts, f"&{node_name} {{", f"missing SPI node {name} in board dts")

    require(board_dts, "&timers4 {", "missing timers4 in board dts")
    require(board_dts, "&timers5 {", "missing timers5 in board dts")
    require(board_dts, "&timers8 {", "missing timers8 in board dts")
    require(board_dts, "&timers12 {", "missing timers12 in board dts")
    require(board_dts, "&i2c2 {\n  status = \"disabled\";", "i2c2 must stay disabled in current Zephyr port")
    require(board_dts, "tim5_ch1_ph10", "tim5 pin mapping must match imported ioc_json")
    require(board_dts, "tim8_ch1_pi5", "tim8 pin mapping must match imported ioc_json")
    require(board_dts, "tim12_ch1_ph6", "tim12 pin mapping must match imported ioc_json")

    for pin_name, label in (
        ("PG1", "LED1"),
        ("PG2", "LED2"),
        ("PG3", "LED3"),
        ("PG4", "LED4"),
        ("PG5", "LED5"),
        ("PG6", "LED6"),
        ("PG7", "LED7"),
        ("PG8", "LED8"),
        ("PH2", "POWER1"),
        ("PH3", "POWER2"),
        ("PH4", "POWER3"),
        ("PH5", "POWER4"),
    ):
        pin_info = gpio_cfg["pins"][pin_name]
        if pin_info.get("label") != label:
            raise SystemExit(f"imported gpio label mismatch for {pin_name}: expected {label}")

    if usb_cfg.get("class_type") != "CDC":
        raise SystemExit("USB class in imported json is not CDC")
    require(prj_conf, "CONFIG_USB_CDC_ACM=y", "USB CDC ACM must be enabled")
    require(board_dts, "&usbotg_fs {", "missing usb otg fs node")

    spi4_mode = spi_cfg["instances"]["SPI4"]["mode"]
    spi5_polarity = spi_cfg["instances"]["SPI5"]["clock_polarity"]
    if spi4_mode != "SPI_MODE_MASTER":
        raise SystemExit("SPI4 mode in imported json is not master")
    if spi5_polarity != "SPI_POLARITY_HIGH":
        raise SystemExit("SPI5 polarity in imported json is not preserved")

    require(app_cmake, "bsp_spi.c", "bsp_spi.c must be compiled")
    require(app_cmake, "bsp_pwm.c", "bsp_pwm.c must be compiled")
    require(app_cmake, "bsp_tim.c", "bsp_tim.c must be compiled")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
