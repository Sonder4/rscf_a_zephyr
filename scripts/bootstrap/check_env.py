from __future__ import annotations

import json
import os
import shutil
import sys
import subprocess
from glob import glob
from pathlib import Path
from typing import Callable


def find_program(name: str, which: Callable[[str], str | None] = shutil.which) -> str | None:
    return which(name)


def read_python_version(python_path: str | None) -> str | None:
    if python_path is None:
        return None

    try:
        result = subprocess.run(
            [python_path, "-c", "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}.{sys.version_info.micro}')"],
            check=True,
            capture_output=True,
            text=True,
        )
    except (OSError, subprocess.CalledProcessError):
        return None

    return result.stdout.strip() or None


def version_at_least(version: str | None, minimum: tuple[int, int]) -> bool:
    if version is None:
        return False

    parts = version.split(".")
    if len(parts) < 2:
        return False

    try:
        current = tuple(int(part) for part in parts[:2])
    except ValueError:
        return False

    return current >= minimum


def find_default_gnuarmemb_path() -> str | None:
    matches = sorted(glob("/opt/st/stm32cubeclt_*/GNU-tools-for-STM32"))
    return matches[-1] if matches else None


def build_report(
    env: dict[str, str] | None = None,
    exists: Callable[[str], bool] | None = None,
    which: Callable[[str], str | None] = shutil.which,
) -> dict[str, dict[str, object]]:
    runtime_env = dict(os.environ if env is None else env)
    path_exists = os.path.exists if exists is None else exists
    python_path = find_program("python3", which)
    python_version = read_python_version(python_path)
    sdk_dir = runtime_env.get("ZEPHYR_SDK_INSTALL_DIR")
    gnuarmemb_dir = runtime_env.get("GNUARMEMB_TOOLCHAIN_PATH") or find_default_gnuarmemb_path()
    gnuarmemb_variant = runtime_env.get("ZEPHYR_TOOLCHAIN_VARIANT") or ("gnuarmemb" if gnuarmemb_dir else None)
    python_ok = python_path is not None and version_at_least(python_version, (3, 12))

    report = {
        "python3": {
            "ok": python_ok,
            "path": python_path,
            "version": python_version,
            "minimum": "3.12",
        },
        "cmake": {
            "ok": find_program("cmake", which) is not None,
            "path": find_program("cmake", which),
        },
        "west": {
            "ok": find_program("west", which) is not None,
            "path": find_program("west", which),
        },
        "zephyr_sdk": {
            "ok": bool(sdk_dir) and path_exists(sdk_dir),
            "path": sdk_dir,
            "source": "env" if sdk_dir else "missing",
        },
        "gnuarmemb": {
            "ok": (gnuarmemb_variant == "gnuarmemb") and bool(gnuarmemb_dir) and path_exists(gnuarmemb_dir),
            "path": gnuarmemb_dir,
            "variant": gnuarmemb_variant,
            "source": "env" if gnuarmemb_dir else "missing",
        },
    }

    return report


def main() -> int:
    report = build_report()
    workspace_root = Path(__file__).resolve().parents[2]
    zephyr_base = os.environ.get("ZEPHYR_BASE")

    print(json.dumps(
        {
            "workspace_root": str(workspace_root),
            "zephyr_base": zephyr_base,
            "report": report,
        },
        indent=2,
        sort_keys=True,
    ))

    tools_ok = report["python3"]["ok"] and report["cmake"]["ok"] and report["west"]["ok"]
    toolchain_ok = report["zephyr_sdk"]["ok"] or report["gnuarmemb"]["ok"]

    if tools_ok and toolchain_ok:
        return 0

    return 1


if __name__ == "__main__":
    sys.exit(main())
