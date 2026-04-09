#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
VENV_DIR="${ROOT_DIR}/.venv"
WORKSPACE_DIR="${ROOT_DIR}/.workspace"
MANIFEST_URL="file://${ROOT_DIR}"

if [[ ! -d "${VENV_DIR}" ]]; then
  python3 -m venv "${VENV_DIR}"
fi

# shellcheck disable=SC1091
source "${VENV_DIR}/bin/activate"

python3 -m pip install --upgrade pip
python3 -m pip install west

if [[ ! -d "${WORKSPACE_DIR}/.west" ]]; then
  west init -m "${MANIFEST_URL}" --mf west.yml -o=--depth=1 "${WORKSPACE_DIR}"
fi

(
  cd "${WORKSPACE_DIR}"
  west update -n -o=--depth=1
  west zephyr-export
  python3 "${ROOT_DIR}/scripts/bootstrap/check_env.py"
)
