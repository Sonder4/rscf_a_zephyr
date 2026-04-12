from __future__ import annotations

import importlib.util
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
MODULE_PATH = REPO_ROOT / "scripts" / "bootstrap" / "check_env.py"


def load_module():
    spec = importlib.util.spec_from_file_location("check_env", MODULE_PATH)
    module = importlib.util.module_from_spec(spec)
    assert spec is not None
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


def test_check_env_exports_expected_helpers() -> None:
    module = load_module()

    assert hasattr(module, "build_report")
    assert hasattr(module, "find_program")
    assert hasattr(module, "main")


def test_build_report_handles_missing_programs() -> None:
    module = load_module()
    module.find_default_gnuarmemb_path = lambda: None
    report = module.build_report(
        env={},
        exists=lambda path: False,
        which=lambda name: None,
    )

    assert report["python3"]["ok"] is False
    assert report["cmake"]["ok"] is False
    assert report["west"]["ok"] is False
    assert report["zephyr_sdk"]["ok"] is False
    assert report["gnuarmemb"]["ok"] is False


def test_build_report_detects_sdk_env_var() -> None:
    module = load_module()
    module.find_default_gnuarmemb_path = lambda: None
    module.read_python_version = lambda _path: "3.12.3"
    report = module.build_report(
        env={"ZEPHYR_SDK_INSTALL_DIR": "/opt/zephyr-sdk-0.17.0"},
        exists=lambda path: True,
        which=lambda name: f"/usr/bin/{name}",
    )

    assert report["python3"]["ok"] is True
    assert report["cmake"]["ok"] is True
    assert report["west"]["ok"] is True
    assert report["zephyr_sdk"]["ok"] is True
    assert report["zephyr_sdk"]["source"] == "env"
    assert report["gnuarmemb"]["ok"] is False


def test_build_report_detects_gnuarmemb_toolchain() -> None:
    module = load_module()
    module.find_default_gnuarmemb_path = lambda: None
    module.read_python_version = lambda _path: "3.12.4"
    report = module.build_report(
        env={
            "ZEPHYR_TOOLCHAIN_VARIANT": "gnuarmemb",
            "GNUARMEMB_TOOLCHAIN_PATH": "/opt/st/stm32cubeclt_1.21.0/GNU-tools-for-STM32",
        },
        exists=lambda path: True,
        which=lambda name: f"/usr/bin/{name}",
    )

    assert report["python3"]["ok"] is True
    assert report["gnuarmemb"]["ok"] is True
    assert report["gnuarmemb"]["variant"] == "gnuarmemb"
