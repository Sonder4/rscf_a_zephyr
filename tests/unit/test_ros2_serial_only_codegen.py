from pathlib import Path
import subprocess
import tempfile


REPO_ROOT = Path(__file__).resolve().parents[2]


def test_codegen_emits_serial_only_ros2_host_package():
    with tempfile.TemporaryDirectory() as temp_dir:
        output_dir = Path(temp_dir) / "ros2_pkg"
        script = REPO_ROOT / "mcu_comm" / "protocol_generator" / "generator.py"
        protocol = REPO_ROOT / "mcu_comm" / "protocol_generator" / "protocol_definition.json"

        subprocess.run(
            [
                "python3",
                str(script),
                "-i",
                str(protocol),
                "-o",
                str(output_dir),
                "--ros2-only",
            ],
            check=True,
            cwd=REPO_ROOT,
        )

        cmake_lists = (output_dir / "CMakeLists.txt").read_text(encoding="utf-8")
        node_hpp = (output_dir / "include" / "node" / "mcu_comm_node.hpp").read_text(encoding="utf-8")
        node_cpp = (output_dir / "src" / "node" / "mcu_comm_node.cpp").read_text(encoding="utf-8")
        launch_py = (output_dir / "launch" / "mcu_one.launch.py").read_text(encoding="utf-8")
        config_json = (output_dir / "config" / "mcu_one_config.json").read_text(encoding="utf-8")

        assert not (output_dir / "include" / "transport" / "can_transport.hpp").exists()
        assert not (output_dir / "src" / "transport" / "can_transport.cpp").exists()
        assert not (output_dir / "include" / "transport" / "tcp_transport.hpp").exists()
        assert not (output_dir / "src" / "transport" / "tcp_transport.cpp").exists()

        assert "soulde_usb2can" not in cmake_lists
        assert "can_transport.cpp" not in cmake_lists
        assert "tcp_transport.cpp" not in cmake_lists

        assert "transport/can_transport.hpp" not in node_hpp
        assert "transport/tcp_transport.hpp" not in node_hpp
        assert "can_device_path_" not in node_hpp
        assert "tcp_host_" not in node_hpp
        assert "transport_type_" not in node_hpp
        assert "fallback_transport_type_" not in node_hpp

        assert "TransportType::Can" not in node_cpp
        assert "TransportType::Tcp" not in node_cpp
        assert 'declare_parameter("transport_type"' not in node_cpp
        assert 'declare_parameter("fallback_transport_type"' not in node_cpp
        assert 'declare_parameter("tcp_host"' not in node_cpp
        assert 'declare_parameter("can_device_path"' not in node_cpp
        assert 'declare_parameter("cmd_send_rate", 100.0);' in node_cpp
        assert "std::make_shared<TcpTransport>" not in node_cpp
        assert "std::make_shared<CanTransport>" not in node_cpp

        assert "transport_type" not in launch_py
        assert "fallback_transport_type" not in launch_py
        assert "tcp_host" not in launch_py
        assert "tcp_port" not in launch_py

        assert '"pc_comm_type": "serial"' in config_json
        assert '"tcp_host"' not in config_json


def test_task4_mcu_side_hooks_do_not_expand_ros2_only_codegen_scope():
    with tempfile.TemporaryDirectory() as temp_dir:
        output_dir = Path(temp_dir) / "ros2_pkg"
        script = REPO_ROOT / "mcu_comm" / "protocol_generator" / "generator.py"
        protocol = REPO_ROOT / "mcu_comm" / "protocol_generator" / "protocol_definition.json"

        subprocess.run(
            [
                "python3",
                str(script),
                "-i",
                str(protocol),
                "-o",
                str(output_dir),
                "--ros2-only",
            ],
            check=True,
            cwd=REPO_ROOT,
        )

        cmake_lists = (output_dir / "CMakeLists.txt").read_text(encoding="utf-8")
        node_hpp = (output_dir / "include" / "node" / "mcu_comm_node.hpp").read_text(encoding="utf-8")
        node_cpp = (output_dir / "src" / "node" / "mcu_comm_node.cpp").read_text(encoding="utf-8")

        assert "rscf_link_usb" not in cmake_lists
        assert "rscf_link_uart" not in cmake_lists
        assert "rscf_link_can" not in cmake_lists
        assert "rscf_link_spi" not in cmake_lists

        assert "Comm_ResolveTransportOrDefault" not in node_hpp
        assert "Comm_OnTransportResolved" not in node_hpp
        assert "RobotInterfaceResolveTransportOverride" not in node_hpp

        assert "Comm_ResolveTransportOrDefault" not in node_cpp
        assert "Comm_OnTransportResolved" not in node_cpp
        assert "RobotInterfaceOnCompatSystemCmd" not in node_cpp
