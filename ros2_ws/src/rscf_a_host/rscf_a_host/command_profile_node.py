import math

import rclpy
from geometry_msgs.msg import Twist
from rclpy.node import Node
from rclpy.executors import ExternalShutdownException
from rclpy.qos import HistoryPolicy, QoSProfile, ReliabilityPolicy
from std_msgs.msg import Float32


class RSCFCommandProfileNode(Node):
    def __init__(self) -> None:
        super().__init__("rscf_command_profile")
        self.declare_parameter("publish_rate_hz", 50.0)
        self.declare_parameter("mode", "step")
        self.declare_parameter("linear_x", 0.35)
        self.declare_parameter("linear_y", 0.0)
        self.declare_parameter("angular_z", 0.25)
        self.declare_parameter("sine_frequency_hz", 0.2)
        self.declare_parameter("send_heading", False)
        self.declare_parameter("heading_deg", 0.0)
        self.declare_parameter("publish_zero_on_shutdown", True)

        best_effort = QoSProfile(
            history=HistoryPolicy.KEEP_LAST,
            depth=10,
            reliability=ReliabilityPolicy.BEST_EFFORT,
        )

        self.cmd_pub = self.create_publisher(Twist, "/cmd_vel", best_effort)
        self.heading_pub = self.create_publisher(Float32, "/rscf/heading_target_deg", best_effort)
        self.start_time = self.get_clock().now()

        publish_rate_hz = max(1.0, float(self.get_parameter("publish_rate_hz").value))
        self.timer = self.create_timer(1.0 / publish_rate_hz, self._publish_command)

    def _elapsed_sec(self) -> float:
        return (self.get_clock().now() - self.start_time).nanoseconds / 1e9

    def _build_twist(self, elapsed_sec: float) -> Twist:
        mode = str(self.get_parameter("mode").value).strip().lower()
        linear_x = float(self.get_parameter("linear_x").value)
        linear_y = float(self.get_parameter("linear_y").value)
        angular_z = float(self.get_parameter("angular_z").value)
        sine_frequency_hz = float(self.get_parameter("sine_frequency_hz").value)

        msg = Twist()
        if mode == "idle":
            return msg

        if mode == "sine":
            phase = 2.0 * math.pi * sine_frequency_hz * elapsed_sec
            msg.linear.x = linear_x * math.sin(phase)
            msg.linear.y = linear_y * math.sin(phase)
            msg.angular.z = angular_z * math.sin(phase)
            return msg

        if mode == "circle":
            msg.linear.x = linear_x
            msg.angular.z = angular_z
            return msg

        msg.linear.x = linear_x
        msg.linear.y = linear_y
        msg.angular.z = angular_z
        return msg

    def _publish_command(self) -> None:
        twist = self._build_twist(self._elapsed_sec())
        self.cmd_pub.publish(twist)

        if bool(self.get_parameter("send_heading").value):
            self.heading_pub.publish(Float32(data=float(self.get_parameter("heading_deg").value)))

    def publish_zero(self) -> None:
        if not bool(self.get_parameter("publish_zero_on_shutdown").value):
            return
        if not rclpy.ok():
            return
        self.cmd_pub.publish(Twist())


def main() -> None:
    rclpy.init()
    node = RSCFCommandProfileNode()
    try:
        rclpy.spin(node)
    except (KeyboardInterrupt, ExternalShutdownException):
        pass
    finally:
        try:
            node.publish_zero()
        finally:
            node.destroy_node()
            if rclpy.ok():
                rclpy.shutdown()
