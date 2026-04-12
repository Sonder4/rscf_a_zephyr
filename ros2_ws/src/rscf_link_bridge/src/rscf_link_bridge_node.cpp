#include <chrono>
#include <string>

#include <geometry_msgs/msg/twist.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <std_msgs/msg/u_int32.hpp>

using namespace std::chrono_literals;

class RscfLinkBridgeNode : public rclcpp::Node {
public:
  RscfLinkBridgeNode()
  : Node("rscf_link_bridge_node")
  {
    device_path_ = declare_parameter<std::string>(
      "device_path",
      "/dev/serial/by-id/usb-ZEPHYR_RSCF_A_CDC_ACM_4843500F00410035-if00");
    baudrate_ = declare_parameter<int>("baudrate", 115200);
    compat_mode_ = declare_parameter<bool>("compat_mode", true);

    odom_pub_ = create_publisher<nav_msgs::msg::Odometry>("/odom", 10);
    imu_pub_ = create_publisher<sensor_msgs::msg::Imu>("/imu/data", 10);
    status_pub_ = create_publisher<std_msgs::msg::UInt32>("/rscf/system_status", 10);
    cmd_sub_ = create_subscription<geometry_msgs::msg::Twist>(
      "/cmd_vel",
      10,
      [this](const geometry_msgs::msg::Twist::SharedPtr msg) {
        last_cmd_vel_ = *msg;
      });

    tick_timer_ = create_wall_timer(20ms, [this]() { this->tick(); });
    RCLCPP_INFO(
      get_logger(),
      "rscf_link_bridge skeleton ready device=%s baudrate=%d compat=%d",
      device_path_.c_str(),
      baudrate_,
      compat_mode_ ? 1 : 0);
  }

private:
  void tick()
  {
    std_msgs::msg::UInt32 status;

    status.data = compat_mode_ ? 0x7009U : 0x0000U;
    status_pub_->publish(status);
  }

  std::string device_path_;
  int baudrate_;
  bool compat_mode_;
  geometry_msgs::msg::Twist last_cmd_vel_;
  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;
  rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub_;
  rclcpp::Publisher<std_msgs::msg::UInt32>::SharedPtr status_pub_;
  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_sub_;
  rclcpp::TimerBase::SharedPtr tick_timer_;
};

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<RscfLinkBridgeNode>());
  rclcpp::shutdown();
  return 0;
}
