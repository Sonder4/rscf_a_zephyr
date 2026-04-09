#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
VENV_DIR="${ROOT_DIR}/.venv"
WORKSPACE_DIR="${ROOT_DIR}/.workspace"
MANIFEST_URL="file://${ROOT_DIR}"
CMSIS_REV="512cc7e895e8491696b61f7ba8066b4a182569b8"
HAL_STM32_REV="286dd285b5bb4fddafdfff27b5405264e5a61bfe"
ZEPHYR_SRC_ARCHIVE="${ZEPHYR_SRC_ARCHIVE:-}"

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

(
  cd "${WORKSPACE_DIR}"
  west zephyr-export
  python3 "${ROOT_DIR}/scripts/bootstrap/check_env.py"
)
