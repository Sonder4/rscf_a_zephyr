from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


def test_ros2_workspace_and_host_package_exist() -> None:
    expected = [
        "ros2_ws/README.md",
        "ros2_ws/src/rscf_a_host/package.xml",
        "ros2_ws/src/rscf_a_host/setup.py",
        "ros2_ws/src/rscf_a_host/setup.cfg",
        "ros2_ws/src/rscf_a_host/launch/rscf_host_io.launch.py",
        "ros2_ws/src/rscf_a_host/scripts/run_agent_usb.sh",
        "ros2_ws/src/rscf_a_host/rscf_a_host/monitor_node.py",
        "ros2_ws/src/rscf_a_host/rscf_a_host/command_profile_node.py",
        "ros2_ws/src/rscf_a_host/rscf_a_host/status.py",
        "ros2_ws/src/rscf_a_host/config/command_profile.yaml",
        "ros2_ws/src/rscf_link_bridge/package.xml",
        "ros2_ws/src/rscf_link_bridge/CMakeLists.txt",
        "ros2_ws/src/rscf_link_bridge/src/rscf_link_bridge_node.cpp",
        "ros2_ws/src/rscf_link_bridge/launch/rscf_link_bridge.launch.py",
    ]

    for relpath in expected:
        assert (REPO_ROOT / relpath).is_file(), relpath


def test_ros2_host_package_exports_expected_interfaces() -> None:
    package_xml = (REPO_ROOT / "ros2_ws/src/rscf_a_host/package.xml").read_text(encoding="utf-8")
    setup_py = (REPO_ROOT / "ros2_ws/src/rscf_a_host/setup.py").read_text(encoding="utf-8")
    launch_py = (REPO_ROOT / "ros2_ws/src/rscf_a_host/launch/rscf_host_io.launch.py").read_text(encoding="utf-8")
    monitor_py = (REPO_ROOT / "ros2_ws/src/rscf_a_host/rscf_a_host/monitor_node.py").read_text(encoding="utf-8")
    command_py = (REPO_ROOT / "ros2_ws/src/rscf_a_host/rscf_a_host/command_profile_node.py").read_text(
        encoding="utf-8"
    )
    status_py = (REPO_ROOT / "ros2_ws/src/rscf_a_host/rscf_a_host/status.py").read_text(encoding="utf-8")

    assert "<build_type>ament_python</build_type>" in package_xml
    assert "<exec_depend>rclpy</exec_depend>" in package_xml
    assert "<exec_depend>nav_msgs</exec_depend>" in package_xml
    assert "<exec_depend>sensor_msgs</exec_depend>" in package_xml
    assert "rscf_monitor = rscf_a_host.monitor_node:main" in setup_py
    assert "rscf_command_profile = rscf_a_host.command_profile_node:main" in setup_py
    assert "run_agent_usb.sh" in launch_py
    assert '"/imu/data"' in monitor_py
    assert '"/odom"' in monitor_py
    assert '"/rscf/system_status"' in monitor_py
    assert '"/cmd_vel"' in command_py
    assert '"/rscf/heading_target_deg"' in command_py
    assert "decode_status_word" in status_py
    assert "service_ready" in status_py
    assert "link_connected" in status_py


def test_ros2_link_bridge_package_exports_cpp_bridge_skeleton() -> None:
    package_xml = (REPO_ROOT / "ros2_ws/src/rscf_link_bridge/package.xml").read_text(encoding="utf-8")
    cmake_lists = (REPO_ROOT / "ros2_ws/src/rscf_link_bridge/CMakeLists.txt").read_text(encoding="utf-8")
    launch_py = (
        REPO_ROOT / "ros2_ws/src/rscf_link_bridge/launch/rscf_link_bridge.launch.py"
    ).read_text(encoding="utf-8")
    node_cpp = (
        REPO_ROOT / "ros2_ws/src/rscf_link_bridge/src/rscf_link_bridge_node.cpp"
    ).read_text(encoding="utf-8")

    assert "<build_type>ament_cmake</build_type>" in package_xml
    assert "<depend>rclcpp</depend>" in package_xml
    assert "add_executable(rscf_link_bridge_node" in cmake_lists
    assert "rscf_link_bridge" in launch_py
    assert 'DeclareLaunchArgument("device_path"' in launch_py
    assert 'DeclareLaunchArgument("compat_mode"' in launch_py
    assert '"/cmd_vel"' in node_cpp
    assert '"/odom"' in node_cpp
    assert '"/imu/data"' in node_cpp
    assert '"/rscf/system_status"' in node_cpp
    assert '"/dev/serial/by-id/' in node_cpp
