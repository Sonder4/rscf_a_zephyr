#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
VENV_DIR="${ROOT_DIR}/.venv"
WORKSPACE_DIR="${ROOT_DIR}/.workspace"
MANIFEST_URL="file://${ROOT_DIR}"
CMSIS_REV="512cc7e895e8491696b61f7ba8066b4a182569b8"
HAL_STM32_REV="286dd285b5bb4fddafdfff27b5405264e5a61bfe"
ZEPHYR_SRC_ARCHIVE="${ZEPHYR_SRC_ARCHIVE:-}"
PYTHON_MIN_VERSION="${PYTHON_MIN_VERSION:-3.12}"

python_version_ok() {
  local python_bin="$1"

  if [[ ! -x "${python_bin}" ]]; then
    return 1
  fi

  "${python_bin}" - "${PYTHON_MIN_VERSION}" <<'PY'
import sys

required = tuple(int(part) for part in sys.argv[1].split("."))
sys.exit(0 if sys.version_info[:len(required)] >= required else 1)
PY
}

ensure_repo_venv() {
  if python_version_ok "${VENV_DIR}/bin/python3"; then
    return 0
  fi

  rm -rf "${VENV_DIR}"

  if command -v uv >/dev/null 2>&1; then
    uv python install "${PYTHON_MIN_VERSION}"
    uv venv --seed --python "${PYTHON_MIN_VERSION}" "${VENV_DIR}"
    return 0
  fi

  if command -v "python${PYTHON_MIN_VERSION}" >/dev/null 2>&1; then
    "python${PYTHON_MIN_VERSION}" -m venv "${VENV_DIR}"
    return 0
  fi

  if python_version_ok "$(command -v python3 2>/dev/null || true)"; then
    python3 -m venv "${VENV_DIR}"
    return 0
  fi

  echo "Python ${PYTHON_MIN_VERSION}+ is required. Install it or provide uv to bootstrap the repo venv." >&2
  return 1
}

extract_zip_tree() {
  local archive_path="$1"
  local destination_dir="$2"
  local stage_dir="${ROOT_DIR}/.bootstrap-artifacts/extract-$(basename "${destination_dir}")"
  local extracted_root

  mkdir -p "${ROOT_DIR}/.bootstrap-artifacts"
  mv "${stage_dir}" "${stage_dir}.old" 2>/dev/null || true
  mkdir -p "${stage_dir}"
  unzip -q "${archive_path}" -d "${stage_dir}"
  extracted_root="$(find "${stage_dir}" -mindepth 1 -maxdepth 1 -type d | head -n 1)"

  if [[ -z "${extracted_root}" ]]; then
    echo "archive extraction failed: ${archive_path}" >&2
    return 1
  fi

  mv "${destination_dir}" "${destination_dir}.old" 2>/dev/null || true
  mkdir -p "${destination_dir}"
  find "${extracted_root}" -mindepth 1 -maxdepth 1 -exec mv {} "${destination_dir}/" \;
}

ensure_archive_project() {
  local archive_url="$1"
  local destination_dir="$2"
  local probe_file="$3"
  local archive_name="$4"
  local archive_path="${ROOT_DIR}/.bootstrap-artifacts/${archive_name}"

  if [[ -f "${destination_dir}/${probe_file}" ]]; then
    return 0
  fi

  curl -fsSL "${archive_url}" -o "${archive_path}"
  extract_zip_tree "${archive_path}" "${destination_dir}"
}

ensure_repo_venv

# shellcheck disable=SC1091
source "${VENV_DIR}/bin/activate"

if [[ -z "${GNUARMEMB_TOOLCHAIN_PATH:-}" ]] && [[ -z "${ZEPHYR_SDK_INSTALL_DIR:-}" ]]; then
  GNUARMEMB_TOOLCHAIN_PATH="$(find /opt/st -maxdepth 2 -type d -path '/opt/st/stm32cubeclt_*/GNU-tools-for-STM32' | sort -V | tail -n 1)"
  if [[ -n "${GNUARMEMB_TOOLCHAIN_PATH}" ]]; then
    export GNUARMEMB_TOOLCHAIN_PATH
    export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
  fi
fi

if [[ -n "${GNUARMEMB_TOOLCHAIN_PATH:-}" ]]; then
  TOOLCHAIN_ROOT="$(cd "${GNUARMEMB_TOOLCHAIN_PATH}/.." && pwd)"
  if [[ -d "${TOOLCHAIN_ROOT}/CMake/bin" ]]; then
    export PATH="${TOOLCHAIN_ROOT}/CMake/bin:${PATH}"
  fi
fi

python3 -m ensurepip --upgrade >/dev/null 2>&1 || true

python3 -m pip install --upgrade pip
python3 -m pip install west colcon-common-extensions vcstool
python3 -m pip install --force-reinstall "setuptools<80" "empy==3.3.4" catkin_pkg lark-parser

if [[ ! -d "${WORKSPACE_DIR}/.west" ]]; then
  west init -m "${MANIFEST_URL}" --mf west.yml -o=--depth=1 "${WORKSPACE_DIR}"
fi

if [[ -f "${ROOT_DIR}/west.yml" ]] && [[ -d "${WORKSPACE_DIR}/manifest" ]]; then
  cp "${ROOT_DIR}/west.yml" "${WORKSPACE_DIR}/manifest/west.yml"
fi

if [[ -n "${ZEPHYR_SRC_ARCHIVE}" ]] && [[ ! -f "${WORKSPACE_DIR}/zephyr/CMakeLists.txt" ]]; then
  extract_zip_tree "${ZEPHYR_SRC_ARCHIVE}" "${WORKSPACE_DIR}/zephyr"
fi

(
  cd "${WORKSPACE_DIR}"
  if [[ -f "${WORKSPACE_DIR}/zephyr/CMakeLists.txt" ]]; then
    west update cmsis hal_stm32 -n -o=--depth=1 || true
  else
    west update zephyr cmsis hal_stm32 -n -o=--depth=1 || true
  fi
)

if [[ ! -f "${WORKSPACE_DIR}/third_party/hal/cmsis/CMSIS/Core/Include/core_cm4.h" ]]; then
  ensure_archive_project \
    "https://codeload.github.com/zephyrproject-rtos/cmsis/zip/${CMSIS_REV}" \
    "${WORKSPACE_DIR}/third_party/hal/cmsis" \
    "CMSIS/Core/Include/core_cm4.h" \
    "cmsis-${CMSIS_REV}.zip"
fi

if [[ ! -f "${WORKSPACE_DIR}/third_party/hal/stm32/stm32cube/stm32f4xx/drivers/include/stm32f4xx_hal.h" ]]; then
  ensure_archive_project \
    "https://codeload.github.com/zephyrproject-rtos/hal_stm32/zip/${HAL_STM32_REV}" \
    "${WORKSPACE_DIR}/third_party/hal/stm32" \
    "stm32cube/stm32f4xx/drivers/include/stm32f4xx_hal.h" \
    "hal_stm32-${HAL_STM32_REV}.zip"
fi

if [[ -f "${WORKSPACE_DIR}/zephyr/scripts/requirements-base.txt" ]]; then
  python3 -m pip install -r "${WORKSPACE_DIR}/zephyr/scripts/requirements-base.txt"
fi

python3 -m pip install jinja2 pyyaml

(
  cd "${WORKSPACE_DIR}"
  west zephyr-export
  python3 "${ROOT_DIR}/scripts/bootstrap/check_env.py"
)
