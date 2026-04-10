import json
import tempfile
import unittest
from pathlib import Path
import importlib.util


GENERATOR_PATH = Path(__file__).resolve().parents[1] / "generator.py"
spec = importlib.util.spec_from_file_location("protocol_generator_module", GENERATOR_PATH)
module = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(module)
ProtocolGenerator = module.ProtocolGenerator


class SystemStatusSchemaTest(unittest.TestCase):
    def _write_protocol_files(self, temp_dir: Path, enum_values):
        protocol = {
            "protocol_version": "2.0.0",
            "min_generator_version": "1.0.0",
            "endianness": "little",
            "frame_format": {
                "header": ["0x52", "0x43"],
                "tail": "0xED",
                "crc": {
                    "enabled": True,
                    "polynomial": "0x1021",
                    "initial": "0xFFFF",
                    "xor_out": "0x0000",
                    "ref_in": False,
                    "ref_out": False,
                },
                "heartbeat": {
                    "enabled": True,
                    "interval_ms": 500,
                    "timeout_ms": 1000,
                },
            },
            "device_config": {
                "pc_hosts": [{"mid": "0x01", "name": "PC_MASTER", "description": "主控PC"}],
                "mcu_devices": [{"mid": "0x05", "name": "MCU_ONE", "description": "一体化MCU"}],
            },
            "transport_config": {
                "default_transport": "uart",
                "default_transport_macro": "MCU_COMM_DEFAULT_TRANSPORT_UART",
                "stm32": {"uart": {"enabled": True, "default_instance": "UART7", "baudrate": 115200}},
                "ros2": {"preferred_transport": "serial", "fallback_transport": "serial"},
            },
            "packets": [
                {
                    "pid": "0x07",
                    "name": "SYSTEM_STATUS",
                    "description": "系统状态同步",
                    "direction": "bidirectional",
                    "status_schema": "system_status_schema.yaml",
                    "status_num_bytes": 2,
                    "data_fields": [],
                    "mid": ["0x05"],
                }
            ],
            "type_mapping": {
                "uint8": {"c_type": "uint8_t", "cpp_type": "uint8_t", "size": 1, "ros_type": "uint8"}
            },
        }
        schema_text = """
enums:
  CtrlMode:
    description: 控制模式
    values:
"""
        for key, value in enum_values.items():
            schema_text += f"      {key}: {value}\n"
        schema_text += """
fields:
  - name: ctrl_mode
    enum: CtrlMode
    sync_direction: bidirectional
    apply_mode: authoritative
    description: 当前控制模式
"""
        protocol_path = temp_dir / "protocol_definition.json"
        schema_path = temp_dir / "system_status_schema.yaml"
        protocol_path.write_text(json.dumps(protocol, ensure_ascii=False, indent=2), encoding="utf-8")
        schema_path.write_text(schema_text, encoding="utf-8")
        return protocol_path

    def test_load_protocol_derives_system_status_layout(self):
        with tempfile.TemporaryDirectory() as temp_dir_str:
            temp_dir = Path(temp_dir_str)
            protocol_path = self._write_protocol_files(temp_dir, {0: "STOP", 1: "FOLLOW", 2: "SPIN"})
            generator = ProtocolGenerator(str(protocol_path), str(temp_dir / "out"))

            self.assertTrue(generator.load_protocol())
            packet = generator.protocol_data["packets"][0]
            self.assertIn("system_status_fields", packet)
            self.assertIn("system_status_layout", packet)
            self.assertEqual(packet["system_status_fields"][0]["bit_width"], 2)
            self.assertEqual(packet["system_status_fields"][0]["sync_direction"], "bidirectional")

    def test_load_protocol_rejects_non_continuous_enum(self):
        with tempfile.TemporaryDirectory() as temp_dir_str:
            temp_dir = Path(temp_dir_str)
            protocol_path = self._write_protocol_files(temp_dir, {0: "STOP", 2: "SPIN"})
            generator = ProtocolGenerator(str(protocol_path), str(temp_dir / "out"))

            self.assertFalse(generator.load_protocol())

    def test_load_protocol_accepts_external_enum_source(self):
        with tempfile.TemporaryDirectory() as temp_dir_str:
            temp_dir = Path(temp_dir_str)
            protocol_path = self._write_protocol_files(temp_dir, {0: "STOP", 1: "FOLLOW", 2: "SPIN"})
            enum_source_path = temp_dir / "control_plane_enums.yaml"
            schema_path = temp_dir / "system_status_schema.yaml"

            enum_source_path.write_text(
                """
enums:
  CtrlMode:
    description: 控制模式
    values:
      0: STOP
      1: FOLLOW
      2: SPIN
""",
                encoding="utf-8",
            )
            schema_path.write_text(
                """
enum_source: control_plane_enums.yaml
fields:
  - name: ctrl_mode
    enum: CtrlMode
    sync_direction: bidirectional
    apply_mode: authoritative
    description: 当前控制模式
""",
                encoding="utf-8",
            )

            generator = ProtocolGenerator(str(protocol_path), str(temp_dir / "out"))

            self.assertTrue(generator.load_protocol())
            packet = generator.protocol_data["packets"][0]
            self.assertIn("system_status_fields", packet)
            self.assertEqual(packet["system_status_fields"][0]["bit_width"], 2)


if __name__ == "__main__":
    unittest.main()
