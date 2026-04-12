#!/usr/bin/env bash
set -euo pipefail

DEFAULT_DEVICE_PATH="/dev/serial/by-id/usb-ZEPHYR_RSCF_A_CDC_ACM_4843500F00410035-if00"
BAUD_RATE="${MICROROS_AGENT_BAUD:-115200}"

if [ $# -ge 1 ]; then
  DEVICE_PATH="$1"
elif [ -e "${DEFAULT_DEVICE_PATH}" ]; then
  DEVICE_PATH="${DEFAULT_DEVICE_PATH}"
else
  DEVICE_PATH="$(find /dev/serial/by-id -maxdepth 1 -type l -name 'usb-*RSCF_A_CDC_ACM*' | head -n 1)"
  if [ -z "${DEVICE_PATH}" ]; then
    DEVICE_PATH="$(ls /dev/ttyACM* 2>/dev/null | head -n 1 || true)"
  fi
fi

source /opt/ros/humble/setup.bash

if ros2 pkg prefix micro_ros_agent >/dev/null 2>&1; then
  if [ -z "${DEVICE_PATH:-}" ] || [ ! -e "${DEVICE_PATH}" ]; then
    echo "未找到 micro-ROS USB 设备，请显式传入设备路径，例如 /dev/ttyACM0" >&2
    exit 1
  fi
  exec ros2 run micro_ros_agent micro_ros_agent serial --dev "${DEVICE_PATH}" -b "${BAUD_RATE}" -v 6
fi

echo "micro_ros_agent 未安装，请先在 /home/xuan/microros_ws 中构建 agent。" >&2
echo "设备路径: ${DEVICE_PATH}" >&2
exit 1
