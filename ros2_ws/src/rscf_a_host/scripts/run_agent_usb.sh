#!/usr/bin/env bash
set -euo pipefail

DEFAULT_DEVICE_PATH="/dev/serial/by-id/usb-ZEPHYR_RSCF_A_CDC_ACM_4843500F00410035-if00"
DEFAULT_AGENT_SETUP="/home/xuan/microros_ws/agent_ws/install/setup.bash"

if [ $# -ge 1 ] && [ -n "${1}" ]; then
  DEVICE_PATH="$1"
elif [ -e "${DEFAULT_DEVICE_PATH}" ]; then
  DEVICE_PATH="${DEFAULT_DEVICE_PATH}"
else
  DEVICE_PATH="$(find /dev/serial/by-id -maxdepth 1 -type l -name 'usb-*RSCF_A_CDC_ACM*' | head -n 1)"
  if [ -z "${DEVICE_PATH}" ]; then
    DEVICE_PATH="$(ls /dev/ttyACM* 2>/dev/null | head -n 1 || true)"
  fi
fi

if [ $# -ge 2 ] && [ -n "${2}" ]; then
  BAUD_RATE="$2"
else
  BAUD_RATE="${MICROROS_AGENT_BAUD:-115200}"
fi

source /opt/ros/humble/setup.bash

if [ -f "${DEFAULT_AGENT_SETUP}" ]; then
  source "${DEFAULT_AGENT_SETUP}"
fi

if ros2 pkg prefix micro_ros_agent >/dev/null 2>&1; then
  if [ -z "${DEVICE_PATH:-}" ] || [ ! -e "${DEVICE_PATH}" ]; then
    echo "未找到 micro-ROS USB 设备，请显式传入设备路径，例如 /dev/ttyACM0" >&2
    exit 1
  fi

  exec ros2 run micro_ros_agent micro_ros_agent serial --dev "${DEVICE_PATH}" -b "${BAUD_RATE}" -v 6
fi

echo "micro_ros_agent 未安装，请先构建 /home/xuan/microros_ws/agent_ws。" >&2
echo "设备路径: ${DEVICE_PATH:-<unset>}" >&2
exit 1
