from __future__ import annotations

import json
import os
import shutil
import sys
from pathlib import Path
from typing import Callable


def find_program(name: str, which: Callable[[str], str | None] = shutil.which) -> str | None:
    return which(name)


def build_report(
    env: dict[str, str] | None = None,
    exists: Callable[[str], bool] | None = None,
    which: Callable[[str], str | None] = shutil.which,
) -> dict[str, dict[str, object]]:
    runtime_env = dict(os.environ if env is None else env)
    path_exists = os.path.exists if exists is None else exists
    sdk_dir = runtime_env.get("ZEPHYR_SDK_INSTALL_DIR")

    report = {
        "python3": {
            "ok": find_program("python3", which) is not None,
            "path": find_program("python3", which),
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

    if all(item["ok"] for item in report.values()):
        return 0

    return 1


if __name__ == "__main__":
    sys.exit(main())
