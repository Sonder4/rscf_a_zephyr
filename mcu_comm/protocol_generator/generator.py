#!/usr/bin/env python3
"""
MCU-ROS2通信协议代码生成器

该脚本根据JSON协议定义文件自动生成STM32端C代码和ROS2端C++代码。

用法:
    python generator.py --input protocol_definition.json --output-dir ../

作者: 协议架构师
日期: 2026-02-08
版本: 1.0.0
"""

import json
import argparse
import os
import sys
import copy
import math
import shutil
from pathlib import Path
from typing import Dict, List, Any, Optional
from jinja2 import Environment, FileSystemLoader, TemplateError
import yaml
import logging

# 配置日志
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)


class ProtocolGenerator:
    """协议代码生成器主类"""
    
    # STM32类型长度映射（以STM32为基准）
    # 支持两种格式：带_t后缀和不带_t后缀
    STM32_TYPE_SIZES = {
        'uint8': 1,
        'uint8_t': 1,
        'int8': 1,
        'int8_t': 1,
        'int16': 2,
        'int16_t': 2,
        'uint16': 2,
        'uint16_t': 2,
        'float': 4,
        'int32': 4,
        'int32_t': 4,
        'uint32': 4,
        'uint32_t': 4,
        'double': 8,
        'int64': 8,
        'int64_t': 8,
        'uint64': 8,
        'uint64_t': 8,
    }

    CONTROL_PLANE_PACKET_CONFIG = {
        'SYSTEM_STATUS': {
            'kind': 'system_status',
            'schema_key': 'status_schema',
            'num_bytes_key': 'status_num_bytes',
        },
        'SYSTEM_CMD': {
            'kind': 'system_cmd',
            'schema_key': 'cmd_schema',
            'num_bytes_key': 'cmd_num_bytes',
        },
    }

    CONTROL_PLANE_DIRECTION_TO_SUFFIX = {
        'mcu_to_pc': 'DIR_MCU_TO_PC',
        'pc_to_mcu': 'DIR_PC_TO_MCU',
        'bidirectional': 'DIR_BIDIRECTIONAL',
    }

    CONTROL_PLANE_APPLY_TO_SUFFIX = {
        'authoritative': 'APPLY_AUTHORITATIVE',
        'passive': 'APPLY_PASSIVE',
    }

    ALLOWED_CONTROL_PLANE_DIRECTIONS = set(CONTROL_PLANE_DIRECTION_TO_SUFFIX.keys())
    ALLOWED_CONTROL_PLANE_APPLY_MODES = set(CONTROL_PLANE_APPLY_TO_SUFFIX.keys())
    
    def __init__(self, protocol_file: str, output_dir: str):
        """
        初始化生成器
        
        Args:
            protocol_file: JSON协议定义文件路径
            output_dir: 输出目录路径
        """
        self.protocol_file = Path(protocol_file).resolve()
        self.output_dir = Path(output_dir).resolve()
        self.protocol_data: Dict[str, Any] = {}
        self.template_dir = Path(__file__).parent / "templates"
        
        # 初始化Jinja2环境
        self.jinja_env = Environment(
            loader=FileSystemLoader(str(self.template_dir)),
            trim_blocks=True,
            lstrip_blocks=True,
            keep_trailing_newline=True
        )
        
        # 添加自定义过滤器
        self.jinja_env.filters['to_camel_case'] = self._to_camel_case
        self.jinja_env.filters['to_snake_case'] = self._to_snake_case
        self.jinja_env.filters['to_pascal_case'] = self._to_pascal_case
        self.jinja_env.filters['to_upper_snake'] = self._to_upper_snake
        self.jinja_env.filters['hex_to_int'] = self._hex_to_int
        self.jinja_env.filters['get_target_mid'] = self._get_target_mid_filter
        
    def load_protocol(self) -> bool:
        """
        加载并验证JSON协议定义文件
        
        Returns:
            bool: 加载成功返回True，否则返回False
        """
        try:
            with open(self.protocol_file, 'r', encoding='utf-8') as f:
                self.protocol_data = json.load(f)
            
            logger.info(f"成功加载协议定义文件: {self.protocol_file}")
            
            # 验证必要字段
            required_fields = ['protocol_version', 'endianness', 'frame_format', 
                             'device_config', 'packets', 'type_mapping']
            for field in required_fields:
                if field not in self.protocol_data:
                    logger.error(f"缺少必要字段: {field}")
                    return False
            
            # 验证数据包定义
            if not self._validate_packets():
                return False

            # 预处理扩展数据包元数据
            if not self._prepare_extended_packets():
                return False

            if not self._prepare_control_plane_bindings():
                return False
            
            # 计算每个数据包的大小
            self._calculate_packet_sizes()
            
            logger.info(f"协议版本: {self.protocol_data['protocol_version']}")
            logger.info(f"数据包数量: {len(self.protocol_data['packets'])}")
            
            return True
            
        except json.JSONDecodeError as e:
            logger.error(f"JSON解析错误: {e}")
            return False
        except FileNotFoundError:
            logger.error(f"文件未找到: {self.protocol_file}")
            return False
        except Exception as e:
            logger.error(f"加载协议定义时出错: {e}")
            return False
    
    def _validate_packets(self) -> bool:
        """验证数据包定义"""
        packets = self.protocol_data.get('packets', [])
        pids = set()
        
        for i, packet in enumerate(packets):
            # 检查必要字段
            if 'pid' not in packet:
                logger.error(f"数据包 {i} 缺少pid字段")
                return False
            if 'name' not in packet:
                logger.error(f"数据包 {i} 缺少name字段")
                return False
            if 'data_fields' not in packet:
                logger.error(f"数据包 {i} 缺少data_fields字段")
                return False
            
            # 检查PID唯一性
            pid = packet['pid']
            if pid in pids:
                logger.error(f"重复的PID: {pid}")
                return False
            pids.add(pid)
            
            # 验证数据字段
            for j, field in enumerate(packet['data_fields']):
                if 'name' not in field or 'type' not in field:
                    logger.error(f"数据包 {packet['name']} 字段 {j} 缺少name或type")
                    return False
                
                # 检查类型是否有效
                if field['type'] not in self.protocol_data['type_mapping']:
                    logger.error(f"未知的数据类型: {field['type']}")
                    return False
        
        return True
    
    def _calculate_packet_sizes(self):
        """计算每个数据包的大小"""
        type_mapping = self.protocol_data['type_mapping']
        
        for packet in self.protocol_data['packets']:
            total_size = 0
            for field in packet['data_fields']:
                field_type = field['type']
                total_size += type_mapping[field_type]['size']
            total_size += self._get_control_plane_num_bytes(packet)
            packet['total_size'] = total_size
            logger.debug(f"数据包 {packet['name']}: {total_size} 字节")
    
    def calculate_pl(self, data_fields: List[Dict[str, Any]], packet: Optional[Dict[str, Any]] = None) -> int:
        """
        根据data_fields计算PL值（Payload Length）
        
        Args:
            data_fields: 数据字段列表，每个字段包含'name'和'type'
            packet: 数据包定义，可选，用于处理扩展字段
            
        Returns:
            int: PL值，即所有字段的STM32类型大小之和
        """
        pl = 0
        for field in data_fields:
            field_type = field.get('type', '')
            if field_type in self.STM32_TYPE_SIZES:
                pl += self.STM32_TYPE_SIZES[field_type]
            else:
                logger.warning(f"未知类型 '{field_type}'，跳过计算")
        if packet is not None:
            pl += self._get_control_plane_num_bytes(packet)
        return pl
    
    def calculate_field_offsets(self, data_fields: List[Dict[str, Any]], packet: Optional[Dict[str, Any]] = None) -> Dict[str, int]:
        """
        为每个字段计算在数据包中的偏移量
        
        Args:
            data_fields: 数据字段列表，每个字段包含'name'和'type'
            packet: 数据包定义，可选，用于处理扩展字段
            
        Returns:
            Dict[str, int]: 字段名到偏移量的映射
        """
        offsets = {}
        current_offset = 0
        for field in data_fields:
            field_name = field.get('name', '')
            field_type = field.get('type', '')
            offsets[field_name] = current_offset
            if field_type in self.STM32_TYPE_SIZES:
                current_offset += self.STM32_TYPE_SIZES[field_type]
            else:
                logger.warning(f"未知类型 '{field_type}'，偏移量计算可能不准确")
        if packet is not None:
            control_plane_num_bytes = self._get_control_plane_num_bytes(packet)
            if control_plane_num_bytes > 0:
                offsets['control_plane_bytes'] = current_offset
        return offsets

    def _prepare_extended_packets(self) -> bool:
        """预处理扩展数据包元数据。"""
        prepared_packets = []
        for packet in self.protocol_data.get('packets', []):
            if self._get_control_plane_config(packet) is not None:
                prepared_packet = self._prepare_control_plane_packet(packet)
                if prepared_packet is None:
                    return False
                prepared_packets.append(prepared_packet)
            else:
                prepared_packets.append(packet)
        self.protocol_data['packets'] = prepared_packets
        return True

    def _get_control_plane_config(self, packet: Dict[str, Any]) -> Optional[Dict[str, str]]:
        """根据数据包名称获取控制面配置。"""
        packet_name = str(packet.get('name', '')).upper()
        return self.CONTROL_PLANE_PACKET_CONFIG.get(packet_name)

    def _get_control_plane_num_bytes(self, packet: Dict[str, Any]) -> int:
        """获取控制面数据包原始字节数。"""
        config = self._get_control_plane_config(packet)
        if config is None:
            return int(packet.get('control_plane_num_bytes', 0))
        return int(packet.get(config['num_bytes_key'], packet.get('control_plane_num_bytes', 0)) or 0)

    def _load_yaml_file(self, file_path: Path, error_prefix: str) -> Optional[Dict[str, Any]]:
        """读取YAML文件。"""
        if not file_path.exists():
            logger.error(f"{error_prefix}文件不存在: {file_path}")
            return None

        try:
            with open(file_path, 'r', encoding='utf-8') as file:
                return yaml.safe_load(file) or {}
        except Exception as exc:
            logger.error(f"读取{error_prefix}失败: {exc}")
            return None

    def _validate_control_plane_enum(self, enum_name: str, enum_values: Dict[Any, Any]) -> bool:
        """校验控制面枚举连续性。"""
        if not enum_values:
            logger.error(f"控制面enum为空: {enum_name}")
            return False
        try:
            keys = sorted(int(key) for key in enum_values.keys())
        except Exception:
            logger.error(f"控制面enum键必须为整数: {enum_name}")
            return False
        if keys[0] != 0:
            logger.error(f"控制面enum必须从0开始: {enum_name}")
            return False
        if keys != list(range(len(keys))):
            logger.error(f"控制面enum值必须连续: {enum_name}")
            return False
        return True

    def _build_control_plane_macro_name(self, packet_name: str, value: str) -> str:
        """构建控制面宏名。"""
        return f"{packet_name}_{value}"

    def _load_control_plane_catalog(self, schema: Dict[str, Any], schema_path: Path) -> Optional[Dict[str, Any]]:
        """加载控制面共享枚举、服务与动作目录。"""
        catalog = {
            'meta': {},
            'enums': {},
            'services': [],
            'actions': [],
        }

        inline_enums = schema.get('enums', {}) or {}
        catalog['enums'].update(copy.deepcopy(inline_enums))

        enum_source = schema.get('enum_source')
        if enum_source:
            enum_source_path = schema_path.parent / str(enum_source)
            enum_source_data = self._load_yaml_file(enum_source_path, '控制面enum_source ')
            if enum_source_data is None:
                return None
            catalog['meta'] = copy.deepcopy(enum_source_data.get('meta', {}))
            catalog['enums'].update(copy.deepcopy(enum_source_data.get('enums', {})))
            catalog['services'] = copy.deepcopy(enum_source_data.get('services', []))
            catalog['actions'] = copy.deepcopy(enum_source_data.get('actions', []))

        if not catalog['enums']:
            logger.error('控制面schema缺少enums定义')
            return None

        return catalog

    def _prepare_control_plane_packet(self, packet: Dict[str, Any]) -> Optional[Dict[str, Any]]:
        """派生控制面数据包元数据。"""
        packet_copy = copy.deepcopy(packet)
        control_plane_config = self._get_control_plane_config(packet_copy)
        if control_plane_config is None:
            return packet_copy

        packet_name = str(packet_copy.get('name', ''))
        schema_path = self.protocol_file.parent / str(packet_copy.get(control_plane_config['schema_key'], ''))
        schema = self._load_yaml_file(schema_path, f"{packet_name} schema")
        if schema is None:
            return None
        catalog = self._load_control_plane_catalog(schema, schema_path)
        if catalog is None:
            return None

        fields = schema.get('fields', [])
        control_plane_num_bytes = int(packet_copy.get(control_plane_config['num_bytes_key'], 0))

        if control_plane_num_bytes <= 0:
            logger.error(f"{packet_name} {control_plane_config['num_bytes_key']}必须大于0")
            return None

        if not fields:
            logger.error(f"{packet_name} schema缺少fields定义")
            return None

        enum_map = {}
        for enum_name, enum_info in catalog['enums'].items():
            enum_values = enum_info.get('values', {})
            if not self._validate_control_plane_enum(enum_name, enum_values):
                return None
            sorted_keys = sorted(int(key) for key in enum_values.keys())
            enum_map[enum_name] = {
                'description': enum_info.get('description', ''),
                'values': {int(key): str(enum_values[key]) for key in enum_values.keys()},
                'entries': [
                    {'value': key, 'name': str(enum_values[key])} for key in sorted_keys
                ],
                'max_value': sorted_keys[-1],
            }

        field_names = set()
        derived_fields = []
        layout = []
        bit_offset = 0

        for field in fields:
            field_name_value = field.get('name', '')
            field_name = str(field_name_value)
            enum_name = str(field.get('enum', ''))
            is_null_field = (field_name_value is None) or (field_name.upper() == 'NULL')
            raw_bits = int(field.get('bits', 0) or 0)
            align_to_byte = bool(field.get('align_to_byte', False) or field.get('byte_align_before', False))
            sync_direction = str(field.get('sync_direction', '')) if not is_null_field else '-'
            apply_mode = str(field.get('apply_mode', '')) if not is_null_field else '-'

            if not field_name:
                logger.error(f'{packet_name} field缺少name')
                return None
            if is_null_field:
                bit_width = raw_bits
                max_value = (1 << bit_width) - 1 if bit_width > 0 else 0
                enum_name = ''
            else:
                if field_name in field_names:
                    logger.error(f"{packet_name} field名称重复: {field_name}")
                    return None
                field_names.add(field_name)
                if enum_name:
                    if enum_name not in enum_map:
                        logger.error(f"{packet_name} field引用了未知enum: {enum_name}")
                        return None
                    max_value = int(enum_map[enum_name]['max_value'])
                    bit_width = max(1, int(math.ceil(math.log2(max_value + 1))))
                else:
                    if raw_bits <= 0:
                        logger.error(f"{packet_name} raw field缺少bits: {field_name}")
                        return None
                    bit_width = raw_bits
                    max_value = (1 << bit_width) - 1
                if sync_direction not in self.ALLOWED_CONTROL_PLANE_DIRECTIONS:
                    logger.error(f"{packet_name} field sync_direction非法: {sync_direction}")
                    return None
                if apply_mode not in self.ALLOWED_CONTROL_PLANE_APPLY_MODES:
                    logger.error(f"{packet_name} field apply_mode非法: {apply_mode}")
                    return None

            if bit_width <= 0:
                logger.error(f"{packet_name} field位宽非法: {field_name}")
                return None
            if align_to_byte:
                while bit_offset % 8 != 0:
                    layout.append({
                        'kind': 'NULL',
                        'bit_offset': bit_offset,
                        'byte_index': bit_offset // 8,
                        'bit_index': bit_offset % 8,
                        'name': 'NULL',
                        'enum': '-',
                        'value_range': '-',
                        'sync_direction': '-',
                        'apply_mode': '-',
                        'description': f'Auto byte align before {field_name}',
                    })
                    bit_offset += 1
            if (bit_offset % 8) + bit_width > 8:
                while bit_offset % 8 != 0:
                    layout.append({
                        'kind': 'NULL',
                        'bit_offset': bit_offset,
                        'byte_index': bit_offset // 8,
                        'bit_index': bit_offset % 8,
                        'name': 'NULL',
                        'enum': '-',
                        'value_range': '-',
                        'sync_direction': '-',
                        'apply_mode': '-',
                        'description': 'Padding / unused',
                    })
                    bit_offset += 1

            if is_null_field:
                for _ in range(bit_width):
                    layout.append({
                        'kind': 'NULL',
                        'bit_offset': bit_offset,
                        'byte_index': bit_offset // 8,
                        'bit_index': bit_offset % 8,
                        'name': 'NULL',
                        'enum': '-',
                        'value_range': '-',
                        'sync_direction': '-',
                        'apply_mode': '-',
                        'description': str(field.get('description', 'NULL / reserved')),
                    })
                    bit_offset += 1
                continue

            derived_field = {
                'name': field_name,
                'enum': enum_name,
                'description': str(field.get('description', '')),
                'sync_direction': sync_direction,
                'apply_mode': apply_mode,
                'bit_offset': bit_offset,
                'bit_width': bit_width,
                'max_legal_value': max_value,
                'value_range': f"0-{max_value}",
                'is_enum_field': bool(enum_name),
                'sync_direction_macro': self._build_control_plane_macro_name(
                    packet_name,
                    self.CONTROL_PLANE_DIRECTION_TO_SUFFIX[sync_direction]
                ),
                'apply_mode_macro': self._build_control_plane_macro_name(
                    packet_name,
                    self.CONTROL_PLANE_APPLY_TO_SUFFIX[apply_mode]
                ),
            }
            derived_fields.append(derived_field)

            for _ in range(bit_width):
                layout.append({
                    'kind': 'FIELD',
                    'bit_offset': bit_offset,
                    'byte_index': bit_offset // 8,
                    'bit_index': bit_offset % 8,
                    'name': field_name,
                    'enum': enum_name,
                    'value_range': f"0-{max_value}",
                    'sync_direction': sync_direction,
                    'apply_mode': apply_mode,
                    'description': derived_field['description'],
                })
                bit_offset += 1

        total_bits = control_plane_num_bytes * 8
        if bit_offset > total_bits:
            logger.error(f'{packet_name} exceeds configured byte capacity')
            return None

        while bit_offset < total_bits:
            layout.append({
                'kind': 'NULL',
                'bit_offset': bit_offset,
                'byte_index': bit_offset // 8,
                'bit_index': bit_offset % 8,
                'name': 'NULL',
                'enum': '-',
                'value_range': '-',
                'sync_direction': '-',
                'apply_mode': '-',
                'description': 'Padding / unused',
            })
            bit_offset += 1

        packet_copy['control_plane_kind'] = control_plane_config['kind']
        packet_copy['control_plane_macro_prefix'] = packet_name
        packet_copy['control_plane_schema'] = {'enums': enum_map, 'fields': derived_fields}
        packet_copy['control_plane_fields'] = derived_fields
        packet_copy['control_plane_layout'] = layout
        packet_copy['control_plane_catalog'] = catalog
        packet_copy['control_plane_packet_direction'] = packet_copy.get('direction', 'bidirectional')
        packet_copy['control_plane_total_bits'] = total_bits
        packet_copy['control_plane_num_bytes'] = control_plane_num_bytes
        packet_copy['control_plane_has_length_field'] = bool(packet_copy.get('data_fields'))
        packet_copy['control_plane_length_field_name'] = (
            str(packet_copy.get('data_fields', [{}])[0].get('name', 'data_len'))
            if packet_copy['control_plane_has_length_field']
            else ''
        )
        packet_copy['control_plane_bytes_field_name'] = 'status_bytes' if control_plane_config['kind'] == 'system_status' else 'cmd_bytes'
        packet_copy['message_type_name'] = self._to_pascal_case(packet_name)

        if control_plane_config['kind'] == 'system_status':
            packet_copy['system_status_schema'] = packet_copy['control_plane_schema']
            packet_copy['system_status_fields'] = derived_fields
            packet_copy['system_status_layout'] = layout
            packet_copy['status_packet_direction'] = packet_copy['control_plane_packet_direction']
            packet_copy['status_total_bits'] = total_bits
        elif control_plane_config['kind'] == 'system_cmd':
            packet_copy['system_cmd_schema'] = packet_copy['control_plane_schema']
            packet_copy['system_cmd_fields'] = derived_fields
            packet_copy['system_cmd_layout'] = layout
            packet_copy['cmd_packet_direction'] = packet_copy['control_plane_packet_direction']
            packet_copy['cmd_total_bits'] = total_bits
        return packet_copy

    def _validate_status_enum(self, enum_name: str, enum_values: Dict[Any, Any]) -> bool:
        """校验SYSTEM_STATUS枚举连续性。"""
        return self._validate_control_plane_enum(enum_name, enum_values)

    def _prepare_control_plane_bindings(self) -> bool:
        """汇总共享控制面目录。"""
        control_plane_packets = [
            packet for packet in self.protocol_data.get('packets', [])
            if packet.get('control_plane_kind') in ('system_cmd', 'system_status')
        ]
        if not control_plane_packets:
            self.protocol_data['control_plane'] = {
                'meta': {},
                'enums': {},
                'services': [],
                'actions': [],
            }
            return True

        merged_catalog: Optional[Dict[str, Any]] = None
        for packet in control_plane_packets:
            catalog = copy.deepcopy(packet.get('control_plane_catalog', {}))
            if not catalog:
                continue
            if merged_catalog is None:
                merged_catalog = catalog
                continue
            if catalog != merged_catalog:
                logger.error('SYSTEM_CMD 与 SYSTEM_STATUS 的共享控制面目录不一致')
                return False

        if merged_catalog is None:
            merged_catalog = {'meta': {}, 'enums': {}, 'services': [], 'actions': []}

        action_enum_entries = {
            str(name): int(value)
            for value, name in (merged_catalog.get('enums', {}).get('ActionId', {}).get('values', {}) or {}).items()
        }
        service_enum_entries = {
            str(name): int(value)
            for value, name in (merged_catalog.get('enums', {}).get('ServiceId', {}).get('values', {}) or {}).items()
        }
        def normalize_enum_entries(enum_name: str) -> Optional[List[Dict[str, Any]]]:
            enum_info = merged_catalog.get('enums', {}).get(enum_name)
            if enum_info is None:
                logger.error(f"控制面引用了未知枚举: {enum_name}")
                return None
            entries = enum_info.get('entries')
            if entries is not None:
                return copy.deepcopy(entries)
            values = enum_info.get('values', {})
            normalized_entries = []
            for value, name in sorted(
                ((int(raw_value), str(raw_name)) for raw_value, raw_name in values.items()),
                key=lambda item: item[0],
            ):
                normalized_entries.append({'value': value, 'name': name})
            return normalized_entries

        normalized_enums: Dict[str, Dict[str, Any]] = {}
        for enum_name, enum_info in (merged_catalog.get('enums', {}) or {}).items():
            enum_entries = normalize_enum_entries(str(enum_name))
            if enum_entries is None:
                return False
            normalized_enums[str(enum_name)] = {
                'description': str(enum_info.get('description', '')),
                'values': copy.deepcopy(enum_info.get('values', {})),
                'entries': enum_entries,
            }

        normalized_services = []
        for service in merged_catalog.get('services', []):
            enum_entry = str(service.get('enum_entry', ''))
            if enum_entry not in service_enum_entries:
                logger.error(f"控制面service引用了未知ServiceId条目: {enum_entry}")
                return False
            normalized_service = {
                **copy.deepcopy(service),
                'id_value': service_enum_entries[enum_entry],
                'snake_name': str(service.get('name', '')),
                'pascal_name': self._to_pascal_case(str(service.get('name', ''))),
            }

            request_enum = service.get('request_enum')
            if request_enum:
                request_enum_entries = normalize_enum_entries(str(request_enum))
                if request_enum_entries is None:
                    return False
                normalized_service['request_enum'] = str(request_enum)
                normalized_service['request_enum_entries'] = request_enum_entries

            normalized_services.append(normalized_service)

        normalized_actions = []
        for action in merged_catalog.get('actions', []):
            enum_entry = str(action.get('enum_entry', ''))
            action_name = str(action.get('name', ''))
            if enum_entry not in action_enum_entries:
                logger.error(f"控制面action引用了未知ActionId条目: {enum_entry}")
                return False
            normalized_action = {
                **copy.deepcopy(action),
                'id_value': action_enum_entries[enum_entry],
                'snake_name': action_name,
                'pascal_name': self._to_pascal_case(action_name),
            }

            goal_enum = action.get('goal_enum')
            if goal_enum:
                goal_enum_entries = normalize_enum_entries(str(goal_enum))
                if goal_enum_entries is None:
                    return False
                normalized_action['goal_enum'] = str(goal_enum)
                normalized_action['goal_enum_entries'] = goal_enum_entries

            feedback_state_enum = action.get('feedback_state_enum')
            if feedback_state_enum:
                feedback_state_enum_entries = normalize_enum_entries(str(feedback_state_enum))
                if feedback_state_enum_entries is None:
                    return False
                normalized_action['feedback_state_enum'] = str(feedback_state_enum)
                normalized_action['feedback_state_enum_entries'] = feedback_state_enum_entries
            normalized_actions.append(normalized_action)

        self.protocol_data['control_plane'] = {
            'meta': merged_catalog.get('meta', {}),
            'enums': normalized_enums,
            'services': normalized_services,
            'actions': normalized_actions,
        }
        return True

    def get_system_status_packet(self, packets: List[Dict[str, Any]]) -> Optional[Dict[str, Any]]:
        """获取SYSTEM_STATUS数据包定义。"""
        for packet in packets:
            if packet.get('name') == 'SYSTEM_STATUS':
                return packet
        return None

    def get_system_cmd_packet(self, packets: List[Dict[str, Any]]) -> Optional[Dict[str, Any]]:
        """获取SYSTEM_CMD数据包定义。"""
        for packet in packets:
            if packet.get('name') == 'SYSTEM_CMD':
                return packet
        return None

    @staticmethod
    def _normalize_mid_list(mid_value: Any) -> List[str]:
        """将数据包归属MID统一转换为字符串列表。"""
        if isinstance(mid_value, list):
            return [str(item) for item in mid_value]
        if mid_value is None:
            return []
        return [str(mid_value)]

    def _get_pc_master_mid(self) -> str:
        """获取主控PC MID，缺省为0x01。"""
        pc_hosts = self.protocol_data.get('device_config', {}).get('pc_hosts', [])
        if pc_hosts:
            return str(pc_hosts[0].get('mid', '0x01'))
        return '0x01'

    def _packet_belongs_to_mcu(self, packet: Dict[str, Any], mcu_mid: str) -> bool:
        """判断数据包是否归属于指定MCU。"""
        packet_mids = self._normalize_mid_list(packet.get('mid'))
        if packet_mids:
            return str(mcu_mid) in packet_mids

        # 兼容旧字段 target_mid
        target_mid = self._normalize_mid_list(packet.get('target_mid'))
        if not target_mid:
            return True
        return str(mcu_mid) in target_mid

    def _filter_packets_for_mcu(self, packets: List[Dict[str, Any]], mcu_mid: str) -> List[Dict[str, Any]]:
        """筛选属于指定MCU的数据包。"""
        filtered = []
        for packet in packets:
            if self._packet_belongs_to_mcu(packet, mcu_mid):
                packet_copy = copy.deepcopy(packet)
                packet_copy['packet_pl'] = self.calculate_pl(packet.get('data_fields', []), packet_copy)
                packet_copy['field_offsets'] = self.calculate_field_offsets(packet.get('data_fields', []), packet_copy)
                packet_copy['mid'] = self._normalize_mid_list(packet.get('mid', packet.get('target_mid')) )
                filtered.append(packet_copy)
        return filtered

    @staticmethod
    def _device_config_filename(mcu_device: Dict[str, Any]) -> str:
        name = str(mcu_device.get('name', 'mcu')).lower()
        return f"{name}_config.json"

    @staticmethod
    def _device_doc_filename(mcu_device: Dict[str, Any]) -> str:
        name = str(mcu_device.get('name', 'mcu')).lower()
        return f"{name}.md"

    
    def generate_stm32_code(self) -> bool:
        """
        生成STM32端代码 - 为每个MCU设备单独生成一套代码
        
        Returns:
            bool: 生成成功返回True，否则返回False
        """
        logger.info("开始生成STM32端代码...")
        
        stm32_output = self.output_dir / "stm32_firmware"
        
        # 为每个MCU设备生成一套代码
        for mcu_device in self.protocol_data['device_config']['mcu_devices']:
            mcu_name = mcu_device['name']
            mcu_mid = mcu_device['mid']
            
            logger.info(f"生成MCU代码: {mcu_name} (MID: {mcu_mid})")
            
            # 创建MCU专属目录
            mcu_dir = stm32_output / mcu_name
            app_dir = mcu_dir / "APP"
            modules_dir = mcu_dir / "Modules"
            comm_manager_dir = modules_dir / "comm_manager"
            core_dir = comm_manager_dir / "core"
            transports_dir = comm_manager_dir / "transports"
            
            # 创建目录结构
            app_dir.mkdir(parents=True, exist_ok=True)
            core_dir.mkdir(parents=True, exist_ok=True)
            transports_dir.mkdir(parents=True, exist_ok=True)
            
            # 仅为当前MCU保留归属它的数据包
            packets_with_metadata = self._filter_packets_for_mcu(self.protocol_data['packets'], mcu_mid)

            mcu_context = {
                'mcu_device': mcu_device,
                'mcu_name': mcu_name,
                'mcu_mid': mcu_mid,
                'pc_master_mid': self._get_pc_master_mid(),
                'protocol': self.protocol_data,
                'packets': packets_with_metadata,
                'control_plane': self.protocol_data.get('control_plane', {}),
                'system_cmd_packet': self.get_system_cmd_packet(packets_with_metadata),
                'system_status_packet': self.get_system_status_packet(packets_with_metadata),
                'frame_format': self.protocol_data['frame_format'],
                'transport_config': self.protocol_data['transport_config'],
                'device_config': self.protocol_data['device_config'],
                'type_mapping': self.protocol_data['type_mapping'],
                'endianness': self.protocol_data['endianness']
            }
            
            # 生成APP层代码
            self._render_template_with_context(
                "stm32/data_def_h.j2", 
                app_dir / "data_def.h",
                mcu_context
            )
            self._render_template_with_context(
                "stm32/data_def_c.j2", 
                app_dir / "data_def.c",
                mcu_context
            )
            self._render_template_with_context(
                "stm32/robot_interface_h.j2", 
                app_dir / "robot_interface.h",
                mcu_context
            )
            self._render_template_with_context(
                "stm32/robot_interface_c.j2", 
                app_dir / "robot_interface.c",
                mcu_context
            )
            
            # 生成Modules层代码
            self._render_template_with_context(
                "stm32/protocol_h.j2", 
                core_dir / "protocol.h",
                mcu_context
            )
            self._render_template_with_context(
                "stm32/protocol_c.j2", 
                core_dir / "protocol.c",
                mcu_context
            )
            self._render_template_with_context(
                "stm32/comm_manager_h.j2", 
                core_dir / "comm_manager.h",
                mcu_context
            )
            self._render_template_with_context(
                "stm32/comm_manager_c.j2", 
                core_dir / "comm_manager.c",
                mcu_context
            )
            self._render_template_with_context(
                "stm32/transport_interface_h.j2", 
                transports_dir / "transport_interface.h",
                mcu_context
            )
            self._render_template_with_context(
                "stm32/transport_usb_h.j2", 
                transports_dir / "transport_usb.h",
                mcu_context
            )
            self._render_template_with_context(
                "stm32/transport_usb_c.j2", 
                transports_dir / "transport_usb.c",
                mcu_context
            )
            self._render_template_with_context(
                "stm32/transport_can_h.j2", 
                transports_dir / "transport_can.h",
                mcu_context
            )
            self._render_template_with_context(
                "stm32/transport_can_c.j2", 
                transports_dir / "transport_can.c",
                mcu_context
            )
            self._render_template_with_context(
                "stm32/transport_uart_h.j2", 
                transports_dir / "transport_uart.h",
                mcu_context
            )
            self._render_template_with_context(
                "stm32/transport_uart_c.j2", 
                transports_dir / "transport_uart.c",
                mcu_context
            )

            if mcu_name == 'MCU_ONE':
                root_app_dir = self.output_dir.parent / 'USER' / 'APP'
                if root_app_dir.exists():
                    shutil.copy2(app_dir / 'data_def.h', root_app_dir / 'data_def.h')
                    shutil.copy2(app_dir / 'data_def.c', root_app_dir / 'data_def.c')
                    shutil.copy2(app_dir / 'robot_interface.h', root_app_dir / 'robot_interface.h')
                    shutil.copy2(app_dir / 'robot_interface.c', root_app_dir / 'robot_interface.c')
                    logger.info(f"同步MCU_ONE APP生成文件到主工程: {root_app_dir}")
        
        return True
    
    def _render_template_with_context(self, template_path: str, output_file: Path, context: dict):
        """使用指定上下文渲染模板，并保留USER CODE区域"""
        try:
            template = self.jinja_env.get_template(template_path)
            output = template.render(**context)
            
            # 如果文件已存在，保留USER CODE区域
            if output_file.exists():
                output = self._preserve_user_code(output_file, output)
            
            with open(output_file, 'w', encoding='utf-8') as f:
                f.write(output)
            
            logger.info(f"生成文件: {output_file}")
            return True
            
        except TemplateError as e:
            logger.error(f"模板渲染错误: {e}")
            return False
        except Exception as e:
            logger.error(f"生成代码时出错: {e}")
            return False
    
    def _preserve_user_code(self, existing_file: Path, new_content: str) -> str:
        """
        保留现有文件中的USER CODE区域
        
        Args:
            existing_file: 现有文件路径
            new_content: 新生成的内容
            
        Returns:
            str: 合并后的内容
        """
        import re
        
        try:
            with open(existing_file, 'r', encoding='utf-8') as f:
                existing_content = f.read()
            
            # 提取现有文件中的USER CODE区域
            # 格式: /* USER CODE BEGIN xxx */ ... /* USER CODE END xxx */
            pattern = r'/\*\s*USER CODE BEGIN\s+(\w+)\s*\*/(.*?)/\*\s*USER CODE END\s+\1\s*\*/'
            existing_user_codes = {}
            
            for match in re.finditer(pattern, existing_content, re.DOTALL):
                tag_name = match.group(1)
                user_code = match.group(2)
                existing_user_codes[tag_name] = user_code
            
            # 如果没有USER CODE区域，直接返回新内容
            if not existing_user_codes:
                return new_content
            
            # 在新内容中替换USER CODE区域
            def replace_user_code(match):
                tag_name = match.group(1)
                if tag_name in existing_user_codes:
                    return f'/* USER CODE BEGIN {tag_name} */{existing_user_codes[tag_name]}/* USER CODE END {tag_name} */'
                return match.group(0)
            
            result = re.sub(pattern, replace_user_code, new_content, flags=re.DOTALL)
            return result
            
        except Exception as e:
            logger.warning(f"保留USER CODE区域时出错: {e}，使用新内容")
            return new_content
    
    def generate_ros2_code(self) -> bool:
        """
        生成ROS2端代码

        Returns:
            bool: 生成成功返回True，否则返回False
        """
        logger.info("开始生成ROS2端代码...")

        ros2_output = self.output_dir
        msg_dir = ros2_output / "msg"
        srv_dir = ros2_output / "srv"
        action_dir = ros2_output / "action"
        include_dir = ros2_output / "include"
        src_dir = ros2_output / "src"
        test_dir = ros2_output / "test"
        launch_dir = ros2_output / "launch"
        config_dir = ros2_output / "config"
        docs_dir = ros2_output / "docs" / "generated"

        msg_dir.mkdir(parents=True, exist_ok=True)
        srv_dir.mkdir(parents=True, exist_ok=True)
        action_dir.mkdir(parents=True, exist_ok=True)
        (include_dir / "protocol").mkdir(parents=True, exist_ok=True)
        (include_dir / "manager").mkdir(parents=True, exist_ok=True)
        (include_dir / "node").mkdir(parents=True, exist_ok=True)
        (include_dir / "transport").mkdir(parents=True, exist_ok=True)
        (src_dir / "protocol").mkdir(parents=True, exist_ok=True)
        (src_dir / "manager").mkdir(parents=True, exist_ok=True)
        (src_dir / "node").mkdir(parents=True, exist_ok=True)
        (src_dir / "transport").mkdir(parents=True, exist_ok=True)
        (src_dir / "tools").mkdir(parents=True, exist_ok=True)
        test_dir.mkdir(parents=True, exist_ok=True)
        launch_dir.mkdir(parents=True, exist_ok=True)
        config_dir.mkdir(parents=True, exist_ok=True)
        docs_dir.mkdir(parents=True, exist_ok=True)

        if not self._generate_ros2_messages(msg_dir):
            return False
        if not self._generate_ros2_services(srv_dir):
            return False
        if not self._generate_ros2_actions(action_dir):
            return False

        packets_with_metadata = []
        for packet in self.protocol_data['packets']:
            packet_copy = copy.deepcopy(packet)
            packet_copy['packet_pl'] = self.calculate_pl(packet.get('data_fields', []), packet_copy)
            packet_copy['field_offsets'] = self.calculate_field_offsets(packet.get('data_fields', []), packet_copy)
            packet_copy['mid'] = self._normalize_mid_list(packet.get('mid', packet.get('target_mid')))
            packets_with_metadata.append(packet_copy)

        context = {
            'protocol': self.protocol_data,
            'packets': packets_with_metadata,
            'control_plane': self.protocol_data.get('control_plane', {}),
            'system_cmd_packet': self.get_system_cmd_packet(packets_with_metadata),
            'system_status_packet': self.get_system_status_packet(packets_with_metadata),
            'frame_format': self.protocol_data['frame_format'],
            'transport_config': self.protocol_data['transport_config'],
            'device_config': self.protocol_data['device_config'],
            'type_mapping': self.protocol_data['type_mapping'],
            'endianness': self.protocol_data['endianness']
        }

        templates_to_generate = [
            ("ros2/protocol_parser_hpp.j2", include_dir / "protocol" / "protocol_parser.hpp"),
            ("ros2/protocol_parser_cpp.j2", src_dir / "protocol" / "protocol_parser.cpp"),
            ("ros2/comm_manager_hpp.j2", include_dir / "manager" / "comm_manager.hpp"),
            ("ros2/comm_manager_cpp.j2", src_dir / "manager" / "comm_manager.cpp"),
            ("ros2/transport_interface_hpp.j2", include_dir / "transport" / "transport_interface.hpp"),
            ("ros2/can_transport_hpp.j2", include_dir / "transport" / "can_transport.hpp"),
            ("ros2/can_transport_cpp.j2", src_dir / "transport" / "can_transport.cpp"),
            ("ros2/serial_transport_hpp.j2", include_dir / "transport" / "serial_transport.hpp"),
            ("ros2/serial_transport_cpp.j2", src_dir / "transport" / "serial_transport.cpp"),
            ("ros2/tcp_transport_hpp.j2", include_dir / "transport" / "tcp_transport.hpp"),
            ("ros2/tcp_transport_cpp.j2", src_dir / "transport" / "tcp_transport.cpp"),
            ("ros2/udp_frame_probe_cpp.j2", src_dir / "tools" / "udp_frame_probe.cpp"),
        ]

        if not self._render_templates(templates_to_generate):
            return False

        if not self.generate_packet_handlers_hpp(include_dir, context):
            logger.error("生成数据包处理器头文件失败")
            return False

        if not self.generate_packet_processor_hpp(include_dir, context):
            logger.error("生成数据包处理器主类头文件失败")
            return False

        if not self.generate_packet_processor_cpp(src_dir, context):
            logger.error("生成数据包处理器主类实现文件失败")
            return False

        self._render_template_with_context(
            "ros2/mcu_comm_node_hpp.j2",
            include_dir / "node" / "mcu_comm_node.hpp",
            context
        )
        self._render_template_with_context(
            "ros2/mcu_comm_node_cpp.j2",
            src_dir / "node" / "mcu_comm_node.cpp",
            context
        )
        self._render_template_with_context(
            "ros2/main_cpp.j2",
            src_dir / "node" / "main.cpp",
            context
        )

        self._render_template_with_context(
            "ros2/CMakeLists_txt.j2",
            ros2_output / "CMakeLists.txt",
            context
        )
        self._render_template_with_context(
            "ros2/package_xml.j2",
            ros2_output / "package.xml",
            context
        )
        self._render_template_with_context(
            "ros2/test_comm_manager_connection_state_cpp.j2",
            test_dir / "test_comm_manager_connection_state.cpp",
            context
        )

        for mcu_device in self.protocol_data['device_config']['mcu_devices']:
            device_context = dict(context)
            device_context['mcu_device'] = mcu_device
            device_context['config_filename'] = self._device_config_filename(mcu_device)
            device_context['device_packets'] = self._filter_packets_for_mcu(self.protocol_data['packets'], mcu_device['mid'])
            device_context['system_cmd_packet'] = self.get_system_cmd_packet(device_context['device_packets'])
            device_context['system_status_packet'] = self.get_system_status_packet(device_context['device_packets'])
            self._render_template_with_context(
                "ros2/launch_py.j2",
                launch_dir / mcu_device['ros2_launch_file'],
                device_context
            )
            self._render_template_with_context(
                "ros2/device_config_json.j2",
                config_dir / self._device_config_filename(mcu_device),
                device_context
            )
            self._render_template_with_context(
                "docs/mcu_device_md.j2",
                docs_dir / self._device_doc_filename(mcu_device),
                device_context
            )

        if context.get('system_status_packet') is not None:
            self._render_template_with_context(
                'docs/system_status_layout_md.j2',
                docs_dir / 'system_status_layout.md',
                context
            )
        if context.get('system_cmd_packet') is not None:
            self._render_template_with_context(
                'docs/system_cmd_layout_md.j2',
                docs_dir / 'system_cmd_layout.md',
                context
            )
        if context.get('control_plane', {}).get('actions'):
            self._render_template_with_context(
                'docs/control_plane_bindings_md.j2',
                docs_dir / 'control_plane_bindings.md',
                context
            )

        return True

    def _generate_ros2_messages(self, msg_dir: Path) -> bool:
        """生成ROS2消息定义文件"""
        try:
            template = self.jinja_env.get_template("ros2/msg.j2")
            
            for packet in self.protocol_data['packets']:
                msg_filename = self._to_pascal_case(packet['name']) + ".msg"
                output_file = msg_dir / msg_filename
                
                output = template.render(
                    packet=packet,
                    control_plane=self.protocol_data.get('control_plane', {}),
                    system_cmd_packet=self.get_system_cmd_packet([packet]),
                    system_status_packet=self.get_system_status_packet([packet]),
                    type_mapping=self.protocol_data['type_mapping']
                )
                
                with open(output_file, 'w', encoding='utf-8') as f:
                    f.write(output)
                
                logger.info(f"生成消息定义: {output_file}")
            
            return True
            
        except TemplateError as e:
            logger.error(f"模板渲染错误: {e}")
            return False

    def _generate_ros2_services(self, srv_dir: Path) -> bool:
        """生成ROS2服务定义文件。"""
        try:
            template = self.jinja_env.get_template("ros2/service.srv.j2")
            for service in self.protocol_data.get('control_plane', {}).get('services', []):
                output_file = srv_dir / f"{service['pascal_name']}.srv"
                output = template.render(
                    service=service,
                    control_plane=self.protocol_data.get('control_plane', {}),
                )
                with open(output_file, 'w', encoding='utf-8') as file:
                    file.write(output)
                logger.info(f"生成服务定义: {output_file}")
            return True
        except TemplateError as exc:
            logger.error(f"服务模板渲染错误: {exc}")
            return False
        except Exception as exc:
            logger.error(f"生成ROS2服务时出错: {exc}")
            return False

    def _generate_ros2_actions(self, action_dir: Path) -> bool:
        """生成ROS2动作定义文件。"""
        try:
            template = self.jinja_env.get_template("ros2/action.action.j2")
            for action in self.protocol_data.get('control_plane', {}).get('actions', []):
                output_file = action_dir / f"{action['pascal_name']}.action"
                output = template.render(
                    action=action,
                    control_plane=self.protocol_data.get('control_plane', {}),
                )
                with open(output_file, 'w', encoding='utf-8') as file:
                    file.write(output)
                logger.info(f"生成动作定义: {output_file}")
            return True
        except TemplateError as exc:
            logger.error(f"动作模板渲染错误: {exc}")
            return False
        except Exception as exc:
            logger.error(f"生成ROS2动作时出错: {exc}")
            return False
        except Exception as e:
            logger.error(f"生成ROS2消息时出错: {e}")
            return False
    
    def _render_templates(self, templates: List[tuple]) -> bool:
        """
        渲染模板列表
        
        Args:
            templates: (模板路径, 输出文件路径) 的列表
            
        Returns:
            bool: 全部生成成功返回True，否则返回False
        """
        try:
            for template_path, output_file in templates:
                template = self.jinja_env.get_template(template_path)
                
                output = template.render(
                    protocol=self.protocol_data,
                    packets=self.protocol_data['packets'],
                    frame_format=self.protocol_data['frame_format'],
                    transport_config=self.protocol_data['transport_config'],
                    device_config=self.protocol_data['device_config'],
                    type_mapping=self.protocol_data['type_mapping'],
                    endianness=self.protocol_data['endianness']
                )
                
                with open(output_file, 'w', encoding='utf-8') as f:
                    f.write(output)
                
                logger.info(f"生成文件: {output_file}")
            
            return True
            
        except TemplateError as e:
            logger.error(f"模板渲染错误: {e}")
            return False
        except Exception as e:
            logger.error(f"生成代码时出错: {e}")
            return False
    
    def generate_packet_handlers_hpp(self, include_dir: Path, context: dict) -> bool:
        """
        生成数据包处理器头文件
        
        Args:
            include_dir: 头文件输出目录
            context: 模板上下文数据
            
        Returns:
            bool: 生成成功返回True，否则返回False
        """
        logger.info("开始生成数据包处理器头文件...")
        
        try:
            # 创建处理器目录
            handlers_dir = include_dir / "processor"
            handlers_dir.mkdir(parents=True, exist_ok=True)
            
            # 生成数据包处理器头文件
            self._render_template_with_context(
                "ros2/packet_handlers_hpp.j2",
                handlers_dir / "packet_handlers.hpp",
                context
            )
            
            logger.info("数据包处理器头文件生成完成")
            return True
            
        except Exception as e:
            logger.error(f"生成数据包处理器头文件时出错: {e}")
            return False
    
    def generate_packet_processor_hpp(self, include_dir: Path, context: dict) -> bool:
        """
        生成数据包处理器主类头文件
        
        Args:
            include_dir: 头文件输出目录
            context: 模板上下文数据
            
        Returns:
            bool: 生成成功返回True，否则返回False
        """
        logger.info("开始生成数据包处理器主类头文件...")
        
        try:
            # 创建处理器目录
            processor_dir = include_dir / "processor"
            processor_dir.mkdir(parents=True, exist_ok=True)
            
            # 生成数据包处理器主类头文件
            self._render_template_with_context(
                "ros2/packet_processor_hpp.j2",
                processor_dir / "packet_processor.hpp",
                context
            )
            
            logger.info("数据包处理器主类头文件生成完成")
            return True
            
        except Exception as e:
            logger.error(f"生成数据包处理器主类头文件时出错: {e}")
            return False
    
    def generate_packet_processor_cpp(self, src_dir: Path, context: dict) -> bool:
        """
        生成数据包处理器主类实现文件
        
        Args:
            src_dir: 源文件输出目录
            context: 模板上下文数据
            
        Returns:
            bool: 生成成功返回True，否则返回False
        """
        logger.info("开始生成数据包处理器主类实现文件...")
        
        try:
            # 创建处理器目录
            processor_dir = src_dir / "processor"
            processor_dir.mkdir(parents=True, exist_ok=True)
            
            # 生成数据包处理器主类实现文件
            self._render_template_with_context(
                "ros2/packet_processor_cpp.j2",
                processor_dir / "packet_processor.cpp",
                context
            )
            
            logger.info("数据包处理器主类实现文件生成完成")
            return True
            
        except Exception as e:
            logger.error(f"生成数据包处理器主类实现文件时出错: {e}")
            return False
    
    @staticmethod
    def _to_camel_case(name: str) -> str:
        """将下划线命名转换为驼峰命名"""
        components = name.split('_')
        return components[0].lower() + ''.join(x.title() for x in components[1:])
    
    @staticmethod
    def _to_snake_case(name: str) -> str:
        """将驼峰命名转换为下划线命名（如果已经是下划线命名或全大写则直接返回小写）"""
        # 如果已经包含下划线，直接转换为小写
        if '_' in name:
            return name.lower()
        
        # 如果全是大写字母（如HEARTBEAT），直接返回小写
        if name.isupper():
            return name.lower()
        
        # 否则进行驼峰到下划线的转换
        result = []
        for i, char in enumerate(name):
            if char.isupper() and i > 0:
                result.append('_')
            result.append(char.lower())
        return ''.join(result)
    
    @staticmethod
    def _to_pascal_case(name: str) -> str:
        """将下划线命名转换为PascalCase（首字母大写的驼峰命名）"""
        # 如果包含下划线（如CHASSIS_CTRL或chassis_ctrl）
        if '_' in name:
            components = name.split('_')
            return ''.join(x.capitalize() for x in components)
        
        # 如果已经是驼峰或PascalCase，确保首字母大写
        if name[0].islower():
            return name[0].upper() + name[1:]
        return name
    
    @staticmethod
    def _to_upper_snake(name: str) -> str:
        """转换为大写下划线命名"""
        return ProtocolGenerator._to_snake_case(name).upper()
    
    @staticmethod
    def _hex_to_int(hex_str: str) -> int:
        """将十六进制字符串转换为整数"""
        if isinstance(hex_str, str):
            return int(hex_str, 16)
        return hex_str

    def _get_target_mid_for_mcu(self, packet: Dict[str, Any], mcu_mid: str) -> str:
        """
        获取PID注册表中使用的默认目标MID。

        mcu_to_pc / bidirectional 默认指向PC_MASTER，pc_to_mcu 默认记录本机MID。
        """
        if packet.get('direction') in ('mcu_to_pc', 'bidirectional'):
            return self._get_pc_master_mid()
        return str(mcu_mid)

    def _get_target_mid_filter(self, packet: Dict[str, Any], mcu_mid: str) -> str:
        """
        Jinja2过滤器：获取数据包对于特定MCU的target_mid值
        
        Args:
            packet: 数据包定义
            mcu_mid: 当前MCU的mid
            
        Returns:
            str: 适合当前MCU的target_mid值
        """
        return self._get_target_mid_for_mcu(packet, mcu_mid)


def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        description='MCU-ROS2通信协议代码生成器',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
    python generator.py --input protocol_definition.json --output-dir ../
    python generator.py -i protocol_definition.json -o ../ --verbose
        """
    )
    
    parser.add_argument(
        '-i', '--input',
        default='protocol_definition.json',
        help='JSON协议定义文件路径 (默认: protocol_definition.json)'
    )
    
    parser.add_argument(
        '-o', '--output-dir',
        default='..',
        help='输出目录路径 (默认: ..)'
    )
    
    parser.add_argument(
        '--stm32-only',
        action='store_true',
        help='仅生成STM32端代码'
    )
    
    parser.add_argument(
        '--ros2-only',
        action='store_true',
        help='仅生成ROS2端代码'
    )
    
    parser.add_argument(
        '-v', '--verbose',
        action='store_true',
        help='启用详细日志输出'
    )
    
    args = parser.parse_args()
    
    # 设置日志级别
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
    
    # 创建生成器实例
    generator = ProtocolGenerator(args.input, args.output_dir)
    
    # 加载协议定义
    if not generator.load_protocol():
        logger.error("加载协议定义失败，退出")
        sys.exit(1)
    
    success = True
    
    # 生成代码
    if not args.ros2_only:
        if not generator.generate_stm32_code():
            logger.error("生成STM32端代码失败")
            success = False
    
    if not args.stm32_only:
        if not generator.generate_ros2_code():
            logger.error("生成ROS2端代码失败")
            success = False
    
    if success:
        logger.info("代码生成完成!")
        sys.exit(0)
    else:
        logger.error("代码生成过程中出现错误")
        sys.exit(1)


if __name__ == '__main__':
    main()
