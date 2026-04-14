from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description() -> LaunchDescription:
    return LaunchDescription(
        [
            DeclareLaunchArgument("device_path",
                                  default_value="/dev/serial/by-id/usb-ZEPHYR_RSCF_A_CDC_ACM_4843500F00410035-if00"),
            DeclareLaunchArgument("baudrate", default_value="115200"),
            DeclareLaunchArgument("compat_mode", default_value="true"),
            Node(
                package="rscf_link_bridge",
                executable="rscf_link_bridge_node",
                name="rscf_link_bridge",
                parameters=[
                    {
                        "device_path": LaunchConfiguration("device_path"),
                        "baudrate": LaunchConfiguration("baudrate"),
                        "compat_mode": LaunchConfiguration("compat_mode"),
                    }
                ],
            ),
        ]
    )
