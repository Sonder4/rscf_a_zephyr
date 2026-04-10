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


class ControlPlaneSchemaTest(unittest.TestCase):
    def _write_protocol_files(
        self,
        temp_dir: Path,
    ) -> Path:
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
                "mcu_devices": [
                    {
                        "mid": "0x05",
                        "name": "MCU_ONE",
                        "description": "一体化MCU",
                        "uart_instance": "UART8",
                        "baudrate": 115200,
                        "ros2_node_name": "mcu_one_node",
                        "ros2_launch_file": "mcu_one.launch.py",
                        "pc_comm_type": "serial",
                        "pc_comm": {
                            "serial_port": "/dev/ttyACM0",
                            "baudrate": 115200,
                            "debug_enabled": False,
                        },
                    }
                ],
            },
            "transport_config": {
                "default_transport": "uart",
                "default_transport_macro": "MCU_COMM_DEFAULT_TRANSPORT_UART",
                "stm32": {
                    "uart": {
                        "enabled": True,
                        "default_instance": "UART8",
                        "baudrate": 115200,
                        "rx_buffer_bytes": 256,
                        "dma_chunk_size": 255,
                    },
                    "can": {
                        "enabled": False,
                        "controller": "CAN1",
                        "bitrate": 1000000,
                        "rx_id": "0x301",
                        "tx_id": "0x302",
                        "stream_buffer_bytes": 128,
                        "work_mode_macro": "CAN_COMM_DEFAULT_WORK_MODE",
                        "offline_timeout_ticks": 10,
                        "rx_timeout_ms": 10.0,
                    },
                    "usb": {"enabled": True},
                },
                "ros2": {
                    "preferred_transport": "serial",
                    "fallback_transport": "serial",
                    "device_path": "/dev/ttyACM0",
                    "tcp_host": "127.0.0.1",
                    "tcp_port": 8899,
                    "tcp_timeout_ms": 1000,
                    "tcp_no_delay": True,
                    "channel": 1,
                    "tx_id": "0x301",
                    "rx_id": "0x302",
                    "read_timeout_us": 10000,
                },
            },
            "packets": [
                {
                    "pid": "0x09",
                    "name": "SYSTEM_CMD",
                    "description": "系统控制命令",
                    "direction": "pc_to_mcu",
                    "cmd_schema": "system_cmd_schema.yaml",
                    "cmd_num_bytes": 3,
                    "data_fields": [],
                    "mid": ["0x05"],
                },
                {
                    "pid": "0x07",
                    "name": "SYSTEM_STATUS",
                    "description": "系统状态同步",
                    "direction": "mcu_to_pc",
                    "status_schema": "system_status_schema.yaml",
                    "status_num_bytes": 5,
                    "data_fields": [],
                    "mid": ["0x05"],
                },
            ],
            "type_mapping": {
                "uint8": {"c_type": "uint8_t", "cpp_type": "uint8_t", "size": 1, "ros_type": "uint8"},
            },
        }

        control_plane_enums = """
meta:
  ticket_bits: 2
  action_progress_bits: 6

enums:
  ImuState:
    description: IMU 状态
    values:
      0: DISABLED
      1: OK
      2: FAULT
  ChassisCtrlMode:
    description: 底盘控制模式
    values:
      0: RELAX
      1: MANUAL
      2: CMD_VEL
      3: BRAKE_STOP
  HeadingMode:
    description: 车头控制模式
    values:
      0: DISABLED
      1: LOCK_ZERO
      2: TARGET_HEADING
      3: FOLLOW_TARGET
  VelocityFrame:
    description: 速度参考系
    values:
      0: WORLD
      1: BODY
  ServiceId:
    description: 轻量服务标识
    values:
      0: NONE
      1: SET_CTRL_MODE
      2: SET_HEADING_MODE
      3: SET_VELOCITY_FRAME
  ServiceResult:
    description: 轻量服务结果
    values:
      0: NONE
      1: OK
      2: REJECT
      3: ERROR
  ActionId:
    description: 轻量动作标识
    values:
      0: NONE
      1: STEP_TRANSITION
      2: ACQUIRE_KFS
      3: PLACE_KFS
  ActionCmd:
    description: 动作控制命令
    values:
      0: START
      1: CANCEL
      2: QUERY
      3: RESERVED
  ActionState:
    description: 动作执行状态
    values:
      0: IDLE
      1: ACCEPTED
      2: RUNNING
      3: CANCELING
      4: SUCCEEDED
      5: CANCELED
      6: ABORTED
      7: REJECTED
  ActionResult:
    description: 动作执行结果
    values:
      0: NONE
      1: OK
      2: FAIL
      3: TIMEOUT
  StepTransitionGoal:
    description: 上下台阶目标
    values:
      0: UP_STEP
      1: DOWN_STEP
  StepTransitionTaskState:
    description: 上下台阶动作阶段
    values:
      0: IDLE
      1: UP_STEP
      2: DOWN_STEP
      3: UP_STEP_DONE
      4: DOWN_STEP_DONE
  AcquireKfsGoal:
    description: 取 KFS 目标位置
    values:
      0: PLUS_20CM
      1: MINUS_20CM
      2: PLUS_40CM
  AcquireKfsTaskState:
    description: 取 KFS 动作阶段
    values:
      0: IDLE
      1: STARTED
      2: APPROACHING
      3: ACQUIRING
      4: ACQUIRE_SUCCESS
      5: ACQUIRE_FAIL
      6: PLACING
      7: PLACE_SUCCESS
      8: PLACE_FAIL
      9: TASK_DONE
      10: TASK_FAIL
  PlaceKfsGoal:
    description: 放置 KFS 动作命令
    values:
      0: PICK_KFS
      1: OPEN_SUCTION
      2: CLOSE_SUCTION
      3: LIFT_KFS
  PlaceKfsTaskState:
    description: 放置 KFS 动作阶段
    values:
      0: IDLE
      1: PICKING_KFS
      2: ACQUIRE_SUCCESS
      3: ACQUIRE_FAIL
      4: PLACING
      5: PLACE_SUCCESS
      6: PLACE_FAIL
      7: OPEN_SUCTION
      8: CLOSE_SUCTION

services:
  - name: set_ctrl_mode
    enum_entry: SET_CTRL_MODE
    request_field_name: ctrl_mode
    request_description: 设置底盘控制模式
    request_enum: ChassisCtrlMode
  - name: set_heading_mode
    enum_entry: SET_HEADING_MODE
    request_field_name: heading_mode
    request_description: 设置车头控制模式
    request_enum: HeadingMode
  - name: set_velocity_frame
    enum_entry: SET_VELOCITY_FRAME
    request_field_name: velocity_frame
    request_description: 设置速度参考系
    request_enum: VelocityFrame

actions:
  - name: step_transition
    enum_entry: STEP_TRANSITION
    goal_field_name: target
    goal_description: 上下台阶目标
    goal_enum: StepTransitionGoal
    feedback_state_enum: StepTransitionTaskState
  - name: acquire_kfs
    enum_entry: ACQUIRE_KFS
    goal_field_name: target
    goal_description: 取 KFS 目标位置
    goal_enum: AcquireKfsGoal
    feedback_state_enum: AcquireKfsTaskState
  - name: place_kfs
    enum_entry: PLACE_KFS
    goal_field_name: command
    goal_description: 放置 KFS 动作命令
    goal_enum: PlaceKfsGoal
    feedback_state_enum: PlaceKfsTaskState
"""

        system_cmd_schema = """
enum_source: control_plane_enums.yaml
fields:
  - name: service_id
    enum: ServiceId
    sync_direction: pc_to_mcu
    apply_mode: passive
    description: 服务标识
  - name: service_arg
    bits: 2
    sync_direction: pc_to_mcu
    apply_mode: passive
    description: 服务参数
  - name: service_ticket
    bits: 2
    sync_direction: pc_to_mcu
    apply_mode: passive
    description: 服务票据
  - name: action_id
    enum: ActionId
    align_to_byte: true
    sync_direction: pc_to_mcu
    apply_mode: passive
    description: 动作标识
  - name: action_cmd
    enum: ActionCmd
    sync_direction: pc_to_mcu
    apply_mode: passive
    description: 动作命令
  - name: action_ticket
    bits: 2
    sync_direction: pc_to_mcu
    apply_mode: passive
    description: 动作票据
  - name: action_param
    bits: 8
    sync_direction: pc_to_mcu
    apply_mode: passive
    description: 动作参数
"""

        system_status_schema = """
enum_source: control_plane_enums.yaml
fields:
  - name: init_done
    bits: 1
    sync_direction: mcu_to_pc
    apply_mode: passive
    description: 初始化完成标志
  - name: odom_ok
    bits: 1
    sync_direction: mcu_to_pc
    apply_mode: passive
    description: odom 状态
  - name: task_all_ok
    bits: 1
    sync_direction: mcu_to_pc
    apply_mode: passive
    description: 任务运行状态
  - name: peer_mcu_comm_ok
    bits: 1
    sync_direction: mcu_to_pc
    apply_mode: passive
    description: 多 MCU 通信状态
  - name: imu_state
    enum: ImuState
    sync_direction: mcu_to_pc
    apply_mode: passive
    description: IMU 状态
  - name: ctrl_mode
    enum: ChassisCtrlMode
    sync_direction: mcu_to_pc
    apply_mode: passive
    description: 当前底盘控制模式
  - name: heading_mode
    enum: HeadingMode
    sync_direction: mcu_to_pc
    apply_mode: passive
    description: 当前车头控制模式
  - name: velocity_frame
    enum: VelocityFrame
    sync_direction: mcu_to_pc
    apply_mode: passive
    description: 当前速度参考系
  - name: service_id
    enum: ServiceId
    align_to_byte: true
    sync_direction: mcu_to_pc
    apply_mode: passive
    description: 服务回显标识
  - name: service_result
    enum: ServiceResult
    sync_direction: mcu_to_pc
    apply_mode: passive
    description: 服务执行结果
  - name: service_ticket
    bits: 2
    sync_direction: mcu_to_pc
    apply_mode: passive
    description: 服务票据回显
  - name: action_id
    enum: ActionId
    align_to_byte: true
    sync_direction: mcu_to_pc
    apply_mode: passive
    description: 当前动作标识
  - name: action_state
    enum: ActionState
    sync_direction: mcu_to_pc
    apply_mode: passive
    description: 当前动作状态
  - name: action_ticket
    bits: 2
    sync_direction: mcu_to_pc
    apply_mode: passive
    description: 动作票据回显
  - name: action_result
    enum: ActionResult
    sync_direction: mcu_to_pc
    apply_mode: passive
    description: 动作结果
  - name: action_progress
    bits: 6
    sync_direction: mcu_to_pc
    apply_mode: passive
    description: 动作粗粒度进度
"""

        protocol_path = temp_dir / "protocol_definition.json"
        (temp_dir / "control_plane_enums.yaml").write_text(control_plane_enums, encoding="utf-8")
        (temp_dir / "system_cmd_schema.yaml").write_text(system_cmd_schema, encoding="utf-8")
        (temp_dir / "system_status_schema.yaml").write_text(system_status_schema, encoding="utf-8")
        protocol_path.write_text(json.dumps(protocol, ensure_ascii=False, indent=2), encoding="utf-8")
        return protocol_path

    def test_load_protocol_derives_control_plane_metadata(self):
        with tempfile.TemporaryDirectory() as temp_dir_str:
            temp_dir = Path(temp_dir_str)
            protocol_path = self._write_protocol_files(temp_dir)
            generator = ProtocolGenerator(str(protocol_path), str(temp_dir / "out"))

            self.assertTrue(generator.load_protocol())
            packets = {packet["name"]: packet for packet in generator.protocol_data["packets"]}
            control_plane = generator.protocol_data["control_plane"]

            self.assertEqual(packets["SYSTEM_CMD"]["control_plane_num_bytes"], 3)
            self.assertEqual(packets["SYSTEM_STATUS"]["control_plane_num_bytes"], 5)
            self.assertEqual(len(control_plane["services"]), 3)
            self.assertEqual(len(control_plane["actions"]), 3)

            cmd_fields = {field["name"]: field for field in packets["SYSTEM_CMD"]["control_plane_fields"]}
            status_fields = {field["name"]: field for field in packets["SYSTEM_STATUS"]["control_plane_fields"]}
            self.assertEqual(cmd_fields["service_id"]["bit_width"], 2)
            self.assertEqual(cmd_fields["action_param"]["bit_width"], 8)
            self.assertEqual(cmd_fields["action_id"]["bit_offset"], 8)
            self.assertEqual(status_fields["imu_state"]["bit_width"], 2)
            self.assertEqual(status_fields["heading_mode"]["bit_width"], 2)
            self.assertEqual(status_fields["action_state"]["bit_width"], 3)
            self.assertEqual(status_fields["service_id"]["bit_offset"], 16)
            self.assertEqual(status_fields["action_id"]["bit_offset"], 24)

            set_heading_mode = next(
                service for service in control_plane["services"] if service["snake_name"] == "set_heading_mode"
            )
            self.assertEqual(set_heading_mode["request_enum"], "HeadingMode")
            self.assertEqual(set_heading_mode["request_enum_entries"][3]["name"], "FOLLOW_TARGET")

            set_ctrl_mode = next(
                service for service in control_plane["services"] if service["snake_name"] == "set_ctrl_mode"
            )
            self.assertEqual(set_ctrl_mode["request_enum"], "ChassisCtrlMode")
            self.assertEqual(set_ctrl_mode["request_enum_entries"][3]["name"], "BRAKE_STOP")

            step_transition = next(
                action for action in control_plane["actions"] if action["snake_name"] == "step_transition"
            )
            self.assertEqual(step_transition["goal_enum"], "StepTransitionGoal")
            self.assertEqual(step_transition["goal_enum_entries"][1]["name"], "DOWN_STEP")
            self.assertEqual(step_transition["feedback_state_enum"], "StepTransitionTaskState")
            self.assertEqual(step_transition["feedback_state_enum_entries"][3]["name"], "UP_STEP_DONE")
            self.assertNotIn("payload_bindings", control_plane)

    def test_generate_outputs_emit_control_plane_only_action_feedback(self):
        with tempfile.TemporaryDirectory() as temp_dir_str:
            temp_dir = Path(temp_dir_str)
            out_dir = temp_dir / "out"
            protocol_path = self._write_protocol_files(temp_dir)
            generator = ProtocolGenerator(str(protocol_path), str(out_dir))

            self.assertTrue(generator.load_protocol())
            self.assertTrue(generator.generate_ros2_code())
            self.assertTrue(generator.generate_stm32_code())

            set_heading_mode_srv = (out_dir / "srv" / "SetHeadingMode.srv").read_text(encoding="utf-8")
            set_ctrl_mode_srv = (out_dir / "srv" / "SetCtrlMode.srv").read_text(encoding="utf-8")
            step_transition_action = (out_dir / "action" / "StepTransition.action").read_text(encoding="utf-8")
            system_status_msg = (out_dir / "msg" / "SystemStatus.msg").read_text(encoding="utf-8")
            system_cmd_msg = (out_dir / "msg" / "SystemCmd.msg").read_text(encoding="utf-8")
            node_hpp = (out_dir / "include" / "node" / "mcu_comm_node.hpp").read_text(encoding="utf-8")
            node_cpp = (out_dir / "src" / "node" / "mcu_comm_node.cpp").read_text(encoding="utf-8")
            serial_transport_cpp = (out_dir / "src" / "transport" / "serial_transport.cpp").read_text(
                encoding="utf-8"
            )
            data_def_h = (out_dir / "stm32_firmware" / "MCU_ONE" / "APP" / "data_def.h").read_text(
                encoding="utf-8"
            )

            self.assertIn("uint8 HEADING_MODE_FOLLOW_TARGET=3", set_heading_mode_srv)
            self.assertIn("uint8 CTRL_MODE_BRAKE_STOP=3", set_ctrl_mode_srv)
            self.assertIn("uint8 TARGET_DOWN_STEP=1", step_transition_action)
            self.assertIn("uint8 TASK_STATE_UP_STEP=1", step_transition_action)
            self.assertIn("uint8 task_state", step_transition_action)
            self.assertNotIn("payload", step_transition_action)
            self.assertIn("uint8[5] status_bytes", system_status_msg)
            self.assertIn("uint8[3] cmd_bytes", system_cmd_msg)
            self.assertIn("uint8 CTRL_MODE_BRAKE_STOP=3", system_status_msg)
            self.assertNotIn("status_len", system_status_msg)
            self.assertNotIn("cmd_len", system_cmd_msg)
            self.assertNotIn("StepTransitionStatus", node_hpp)
            self.assertNotIn("AcquireKfsStatus", node_hpp)
            self.assertNotIn("PlaceKfsStatus", node_hpp)
            self.assertNotIn("BatteryStatus", node_hpp)
            self.assertIn("std_msgs/msg/float32.hpp", node_hpp)
            self.assertIn("heading_target_subscriber_", node_hpp)
            self.assertIn("float latest_heading_target_deg_{0.0f};", node_hpp)
            self.assertIn("void onHeadingTarget", node_hpp)
            self.assertNotIn("BATTERY_STATUS", node_cpp)
            self.assertNotIn("host_reset_request", node_cpp)
            self.assertNotIn("sync_state", node_cpp)
            self.assertIn('create_subscription<std_msgs::msg::Float32>', node_cpp)
            self.assertIn('"~/heading_target_deg"', node_cpp)
            self.assertIn("latest_heading_target_deg_", node_cpp)
            self.assertIn("cfmakeraw(&options);", serial_transport_cpp)
            self.assertIn("CONTROL_PLANE_HEADING_MODE_FOLLOW_TARGET = 3", data_def_h)
            self.assertIn("CONTROL_PLANE_STEP_TRANSITION_TASK_STATE_UP_STEP_DONE = 3", data_def_h)
            self.assertIn("CONTROL_PLANE_CHASSIS_CTRL_MODE_BRAKE_STOP = 3", data_def_h)


if __name__ == "__main__":
    unittest.main()
