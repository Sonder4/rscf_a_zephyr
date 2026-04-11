#!/usr/bin/env python3
"""
Zephyr 侧 mcu_comm 代码生成包装器。
"""

from __future__ import annotations

import argparse
import importlib.util
import logging
import shutil
import sys
from pathlib import Path


LOGGER = logging.getLogger("rscf_mcu_comm_codegen")

REPO_ROOT = Path(__file__).resolve().parents[2]
GENERATOR_PY = REPO_ROOT / "mcu_comm" / "protocol_generator" / "generator.py"
PROTOCOL_JSON = REPO_ROOT / "mcu_comm" / "protocol_generator" / "protocol_definition.json"


def load_generator_class():
    spec = importlib.util.spec_from_file_location("rscf_protocol_generator", GENERATOR_PY)
    module = importlib.util.module_from_spec(spec)
    assert spec is not None
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module.ProtocolGenerator


class ZephyrProtocolGenerator(load_generator_class()):
    """Zephyr 输出封装。"""

    def build_zephyr_context(self, device_name: str) -> dict:
        for mcu_device in self.protocol_data["device_config"]["mcu_devices"]:
            if str(mcu_device["name"]).upper() != device_name.upper():
                continue

            packets = self._filter_packets_for_mcu(self.protocol_data["packets"], mcu_device["mid"])
            transport_config = self.protocol_data.get("transport_config", {})
            default_transport = str(
                transport_config.get("default_transport", mcu_device.get("comm_type", "usb"))
            ).lower()
            if default_transport in {"uart", "usart", "serial"}:
                default_transport_macro = "MCU_COMM_TRANSPORT_UART"
            elif default_transport == "can":
                default_transport_macro = "MCU_COMM_TRANSPORT_CAN"
            else:
                default_transport_macro = "MCU_COMM_TRANSPORT_USB"

            return {
                "mcu_device": mcu_device,
                "mcu_name": mcu_device["name"],
                "mcu_mid": mcu_device["mid"],
                "pc_master_mid": self._get_pc_master_mid(),
                "protocol": self.protocol_data,
                "packets": packets,
                "control_plane": self.protocol_data.get("control_plane", {}),
                "system_cmd_packet": self.get_system_cmd_packet(packets),
                "system_status_packet": self.get_system_status_packet(packets),
                "frame_format": self.protocol_data["frame_format"],
                "transport_config": self.protocol_data["transport_config"],
                "device_config": self.protocol_data["device_config"],
                "type_mapping": self.protocol_data["type_mapping"],
                "endianness": self.protocol_data["endianness"],
                "default_transport_macro": default_transport_macro,
            }

        raise ValueError(f"unknown MCU device: {device_name}")

    def generate_zephyr_code(self, device_name: str, output_dir: Path) -> bool:
        context = self.build_zephyr_context(device_name)
        app_dir = output_dir / "app"
        core_dir = output_dir / "core"
        transports_dir = output_dir / "transports"

        app_dir.mkdir(parents=True, exist_ok=True)
        core_dir.mkdir(parents=True, exist_ok=True)
        transports_dir.mkdir(parents=True, exist_ok=True)

        templates = [
            ("zephyr/data_def_h.j2", app_dir / "data_def.h"),
            ("zephyr/data_def_c.j2", app_dir / "data_def.c"),
            ("zephyr/robot_interface_h.j2", app_dir / "robot_interface.h"),
            ("zephyr/robot_interface_c.j2", app_dir / "robot_interface.c"),
            ("zephyr/protocol_h.j2", core_dir / "protocol.h"),
            ("zephyr/protocol_c.j2", core_dir / "protocol.c"),
            ("zephyr/comm_manager_h.j2", core_dir / "comm_manager.h"),
            ("zephyr/comm_manager_c.j2", core_dir / "comm_manager.c"),
            ("zephyr/transport_interface_h.j2", transports_dir / "transport_interface.h"),
            ("zephyr/transport_uart_h.j2", transports_dir / "transport_uart.h"),
            ("zephyr/transport_uart_c.j2", transports_dir / "transport_uart.c"),
            ("zephyr/transport_can_h.j2", transports_dir / "transport_can.h"),
            ("zephyr/transport_can_c.j2", transports_dir / "transport_can.c"),
            ("zephyr/transport_usb_h.j2", transports_dir / "transport_usb.h"),
            ("zephyr/transport_usb_c.j2", transports_dir / "transport_usb.c"),
        ]

        for template_path, output_file in templates:
            if not self._render_template_with_context(template_path, output_file, context):
                return False

        return True


def sync_generated_tree(source_dir: Path, sync_dir: Path, device_name: str) -> None:
    target_dir = sync_dir / device_name
    target_dir.parent.mkdir(parents=True, exist_ok=True)
    shutil.copytree(source_dir, target_dir, dirs_exist_ok=True)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="RSCF Zephyr mcu_comm 代码生成")
    parser.add_argument("--input", default=str(PROTOCOL_JSON), help="协议定义 JSON 路径")
    parser.add_argument("--device", default="MCU_ONE", help="目标 MCU 设备名")
    parser.add_argument("--output-dir", required=True, help="Zephyr 生成输出目录")
    parser.add_argument("--sync-dir", default=None, help="显式同步目录")
    parser.add_argument("--sync", action="store_true", help="同步生成结果到 sync-dir")
    parser.add_argument("--ros2-output-dir", default=None, help="可选 ROS2 生成输出目录")
    parser.add_argument("--verbose", action="store_true", help="输出详细日志")
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(asctime)s - %(levelname)s - %(message)s",
    )

    output_dir = Path(args.output_dir).resolve()
    generator = ZephyrProtocolGenerator(args.input, str(output_dir))
    if not generator.load_protocol():
        LOGGER.error("加载协议定义失败")
        return 1

    if not generator.generate_zephyr_code(args.device, output_dir):
        LOGGER.error("生成 Zephyr 通信代码失败")
        return 1

    if args.ros2_output_dir is not None:
        ros2_dir = Path(args.ros2_output_dir).resolve()
        generator.output_dir = ros2_dir
        if not generator.generate_ros2_code():
            LOGGER.error("生成 ROS2 通信代码失败")
            return 1

    if args.sync:
        if args.sync_dir is None:
            LOGGER.error("启用 --sync 时必须同时提供 --sync-dir")
            return 1

        sync_generated_tree(output_dir, Path(args.sync_dir).resolve(), args.device)

    return 0


if __name__ == "__main__":
    sys.exit(main())
