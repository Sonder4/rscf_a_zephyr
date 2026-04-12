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
