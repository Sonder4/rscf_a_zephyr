from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node

from ament_index_python.packages import get_package_share_directory


def generate_launch_description() -> LaunchDescription:
    package_share = get_package_share_directory("rscf_a_host")
    agent_script = PathJoinSubstitution([package_share, "scripts", "run_agent_usb.sh"])
    command_profile = PathJoinSubstitution([package_share, "config", "command_profile.yaml"])

    return LaunchDescription(
        [
            DeclareLaunchArgument("start_agent", default_value="false"),
            DeclareLaunchArgument("start_monitor", default_value="true"),
            DeclareLaunchArgument("start_commander", default_value="false"),
            DeclareLaunchArgument("serial_device", default_value=""),
            DeclareLaunchArgument("baud_rate", default_value="115200"),
            DeclareLaunchArgument("mode", default_value="step"),
            DeclareLaunchArgument("linear_x", default_value="0.35"),
            DeclareLaunchArgument("linear_y", default_value="0.0"),
            DeclareLaunchArgument("angular_z", default_value="0.25"),
            DeclareLaunchArgument("send_heading", default_value="false"),
            DeclareLaunchArgument("heading_deg", default_value="0.0"),
            ExecuteProcess(
                condition=IfCondition(LaunchConfiguration("start_agent")),
                cmd=[
                    "bash",
                    agent_script,
                    LaunchConfiguration("serial_device"),
                    LaunchConfiguration("baud_rate"),
                ],
                output="screen",
            ),
            Node(
                condition=IfCondition(LaunchConfiguration("start_monitor")),
                package="rscf_a_host",
                executable="rscf_monitor",
                name="rscf_monitor",
                output="screen",
            ),
            Node(
                condition=IfCondition(LaunchConfiguration("start_commander")),
                package="rscf_a_host",
                executable="rscf_command_profile",
                name="rscf_command_profile",
                output="screen",
                parameters=[
                    command_profile,
                    {
                        "mode": LaunchConfiguration("mode"),
                        "linear_x": LaunchConfiguration("linear_x"),
                        "linear_y": LaunchConfiguration("linear_y"),
                        "angular_z": LaunchConfiguration("angular_z"),
                        "send_heading": LaunchConfiguration("send_heading"),
                        "heading_deg": LaunchConfiguration("heading_deg"),
                    },
                ],
            ),
        ]
    )
