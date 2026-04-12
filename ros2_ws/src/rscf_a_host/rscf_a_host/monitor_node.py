import math
from dataclasses import dataclass

import rclpy
from geometry_msgs.msg import Twist
from nav_msgs.msg import Odometry
from rclpy.node import Node
from rclpy.qos import HistoryPolicy, QoSProfile, ReliabilityPolicy
from sensor_msgs.msg import Imu
from std_msgs.msg import Float32, UInt32

from .status import decode_status_word


@dataclass
class TopicWindow:
    count: int = 0


class RSCFMonitorNode(Node):
    def __init__(self) -> None:
        super().__init__("rscf_monitor")
        self.declare_parameter("summary_period_sec", 1.0)

        best_effort = QoSProfile(
            history=HistoryPolicy.KEEP_LAST,
            depth=10,
            reliability=ReliabilityPolicy.BEST_EFFORT,
        )
        reliable = QoSProfile(
            history=HistoryPolicy.KEEP_LAST,
            depth=10,
            reliability=ReliabilityPolicy.RELIABLE,
        )

        self.imu_window = TopicWindow()
        self.odom_window = TopicWindow()
        self.status_window = TopicWindow()
        self.cmd_window = TopicWindow()
        self.heading_window = TopicWindow()

        self.last_status = decode_status_word(0)
        self.last_odom_vx = 0.0
        self.last_odom_vy = 0.0
        self.last_odom_wz = 0.0
        self.last_imu_yaw_rate = 0.0
        self.last_heading_deg = math.nan
        self.last_cmd = Twist()

        self.create_subscription(Imu, "/imu/data", self._on_imu, best_effort)
        self.create_subscription(Odometry, "/odom", self._on_odom, reliable)
        self.create_subscription(UInt32, "/rscf/system_status", self._on_status, 10)
        self.create_subscription(Twist, "/cmd_vel", self._on_cmd, best_effort)
        self.create_subscription(Float32, "/rscf/heading_target_deg", self._on_heading, best_effort)

        period = float(self.get_parameter("summary_period_sec").value)
        self.summary_timer = self.create_timer(period, self._emit_summary)

    def _on_imu(self, msg: Imu) -> None:
        self.imu_window.count += 1
        self.last_imu_yaw_rate = float(msg.angular_velocity.z)

    def _on_odom(self, msg: Odometry) -> None:
        self.odom_window.count += 1
        self.last_odom_vx = float(msg.twist.twist.linear.x)
        self.last_odom_vy = float(msg.twist.twist.linear.y)
        self.last_odom_wz = float(msg.twist.twist.angular.z)

    def _on_status(self, msg: UInt32) -> None:
        self.status_window.count += 1
        self.last_status = decode_status_word(msg.data)

    def _on_cmd(self, msg: Twist) -> None:
        self.cmd_window.count += 1
        self.last_cmd = msg

    def _on_heading(self, msg: Float32) -> None:
        self.heading_window.count += 1
        self.last_heading_deg = float(msg.data)

    def _emit_summary(self) -> None:
        period = self.summary_timer.timer_period_ns / 1e9
        status = self.last_status
        heading_text = "nan" if math.isnan(self.last_heading_deg) else f"{self.last_heading_deg:.1f}"
        self.get_logger().info(
            "imu=%.1fHz odom=%.1fHz status=%.1fHz cmd=%.1fHz heading=%.1fHz "
            "odom(vx=%.3f vy=%.3f wz=%.3f) imu(wz=%.3f) cmd(vx=%.3f vy=%.3f wz=%.3f) "
            "status(raw=0x%08X legacy=0x%06X ready=%d link=%d sim=%d imu=%d odom=%d arm=%d heading=%s)"
            % (
                self.imu_window.count / period,
                self.odom_window.count / period,
                self.status_window.count / period,
                self.cmd_window.count / period,
                self.heading_window.count / period,
                self.last_odom_vx,
                self.last_odom_vy,
                self.last_odom_wz,
                self.last_imu_yaw_rate,
                float(self.last_cmd.linear.x),
                float(self.last_cmd.linear.y),
                float(self.last_cmd.angular.z),
                status.raw,
                status.legacy_status,
                int(status.service_ready),
                int(status.link_connected),
                int(status.sim_mode),
                int(status.imu_ready),
                int(status.odom_ready),
                status.arm_state,
                heading_text,
            )
        )

        self.imu_window.count = 0
        self.odom_window.count = 0
        self.status_window.count = 0
        self.cmd_window.count = 0
        self.heading_window.count = 0


def main() -> None:
    rclpy.init()
    node = RSCFMonitorNode()
    try:
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()
