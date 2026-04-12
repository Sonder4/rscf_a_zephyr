from pathlib import Path
import importlib.util
import subprocess
import sys
import textwrap
import types


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


def test_rscf_link_bridge_launch_file_builds_expected_actions():
    launch_path = REPO_ROOT / "ros2_ws/src/rscf_link_bridge/launch/rscf_link_bridge.launch.py"

    class LaunchDescription:
        def __init__(self, entities):
            self.entities = entities

    class DeclareLaunchArgument:
        def __init__(self, name, default_value=None):
            self.name = name
            self.default_value = default_value

    class LaunchConfiguration:
        def __init__(self, name):
            self.name = name

    class Node:
        def __init__(self, **kwargs):
            self.kwargs = kwargs

    launch_module = types.ModuleType("launch")
    launch_actions = types.ModuleType("launch.actions")
    launch_substitutions = types.ModuleType("launch.substitutions")
    launch_ros_module = types.ModuleType("launch_ros")
    launch_ros_actions = types.ModuleType("launch_ros.actions")

    launch_module.LaunchDescription = LaunchDescription
    launch_actions.DeclareLaunchArgument = DeclareLaunchArgument
    launch_substitutions.LaunchConfiguration = LaunchConfiguration
    launch_ros_actions.Node = Node

    saved_modules = {}
    for name, module in {
        "launch": launch_module,
        "launch.actions": launch_actions,
        "launch.substitutions": launch_substitutions,
        "launch_ros": launch_ros_module,
        "launch_ros.actions": launch_ros_actions,
    }.items():
        saved_modules[name] = sys.modules.get(name)
        sys.modules[name] = module

    try:
        spec = importlib.util.spec_from_file_location("rscf_link_bridge_launch", launch_path)
        module = importlib.util.module_from_spec(spec)
        assert spec is not None
        assert spec.loader is not None
        spec.loader.exec_module(module)
        description = module.generate_launch_description()
    finally:
        for name, previous in saved_modules.items():
            if previous is None:
                sys.modules.pop(name, None)
            else:
                sys.modules[name] = previous

    assert isinstance(description, LaunchDescription)
    arguments = [entity for entity in description.entities if isinstance(entity, DeclareLaunchArgument)]
    node = next(entity for entity in description.entities if isinstance(entity, Node))

    assert [argument.name for argument in arguments] == ["device_path", "baudrate", "compat_mode"]
    assert node.kwargs["package"] == "rscf_link_bridge"
    assert node.kwargs["executable"] == "rscf_link_bridge_node"
    assert node.kwargs["parameters"][0]["device_path"].name == "device_path"
    assert node.kwargs["parameters"][0]["baudrate"].name == "baudrate"
    assert node.kwargs["parameters"][0]["compat_mode"].name == "compat_mode"


def test_rscf_link_bridge_node_cpp_runs_with_stub_rclcpp(tmp_path):
    stub_root = tmp_path / "stubs"
    (stub_root / "rclcpp").mkdir(parents=True)
    (stub_root / "geometry_msgs" / "msg").mkdir(parents=True)
    (stub_root / "nav_msgs" / "msg").mkdir(parents=True)
    (stub_root / "sensor_msgs" / "msg").mkdir(parents=True)
    (stub_root / "std_msgs" / "msg").mkdir(parents=True)

    (stub_root / "rclcpp" / "rclcpp.hpp").write_text(
        textwrap.dedent(
            """
            #ifndef RCLCPP_RCLCPP_HPP_
            #define RCLCPP_RCLCPP_HPP_
            #include <chrono>
            #include <cstdint>
            #include <functional>
            #include <memory>
            #include <string>
            #include <unordered_map>
            #include <utility>
            #include <vector>

            namespace rclcpp {

            class Logger {};

            class TimerBase {
            public:
              using SharedPtr = std::shared_ptr<TimerBase>;
              explicit TimerBase(std::function<void()> callback)
              : callback_(std::move(callback)) {}
              void fire() { callback_(); }
            private:
              std::function<void()> callback_;
            };

            template<typename MsgT>
            class Publisher {
            public:
              using SharedPtr = std::shared_ptr<Publisher<MsgT>>;
              explicit Publisher(std::string topic) : topic_(std::move(topic)) {}
              void publish(const MsgT &msg)
              {
                last_msg = msg;
                publish_count++;
              }
              std::string topic_;
              MsgT last_msg{};
              uint32_t publish_count{0};
            };

            template<typename MsgT>
            class Subscription {
            public:
              using SharedPtr = std::shared_ptr<Subscription<MsgT>>;
              explicit Subscription(std::string topic) : topic_(std::move(topic)) {}
              std::string topic_;
            };

            class Node {
            public:
              explicit Node(std::string name) : name_(std::move(name)) {}
              Logger get_logger() const { return Logger{}; }

              template<typename T>
              T declare_parameter(const std::string &name, const T &default_value)
              {
                if constexpr (std::is_same_v<T, std::string>) {
                  string_params_[name] = default_value;
                } else if constexpr (std::is_same_v<T, int>) {
                  int_params_[name] = default_value;
                } else if constexpr (std::is_same_v<T, bool>) {
                  bool_params_[name] = default_value;
                }
                return default_value;
              }

              template<typename MsgT>
              typename Publisher<MsgT>::SharedPtr create_publisher(const std::string &topic, size_t)
              {
                auto publisher = std::make_shared<Publisher<MsgT>>(topic);
                publishers_[topic] = publisher;
                return publisher;
              }

              template<typename MsgT, typename CallbackT>
              typename Subscription<MsgT>::SharedPtr create_subscription(const std::string &topic, size_t, CallbackT)
              {
                auto subscription = std::make_shared<Subscription<MsgT>>(topic);
                subscriptions_.push_back(topic);
                return subscription;
              }

              template<typename Rep, typename Period, typename CallbackT>
              TimerBase::SharedPtr create_wall_timer(std::chrono::duration<Rep, Period>, CallbackT callback)
              {
                auto timer = std::make_shared<TimerBase>(callback);
                timers_.push_back(timer);
                return timer;
              }

              template<typename MsgT>
              std::shared_ptr<Publisher<MsgT>> find_publisher(const std::string &topic) const
              {
                auto it = publishers_.find(topic);
                if (it == publishers_.end()) {
                  return nullptr;
                }
                return std::static_pointer_cast<Publisher<MsgT>>(it->second);
              }

              bool has_subscription(const std::string &topic) const
              {
                for (const auto &entry : subscriptions_) {
                  if (entry == topic) {
                    return true;
                  }
                }
                return false;
              }

              std::string get_string_param(const std::string &name) const { return string_params_.at(name); }
              int get_int_param(const std::string &name) const { return int_params_.at(name); }
              bool get_bool_param(const std::string &name) const { return bool_params_.at(name); }

              void run_timers_once()
              {
                for (const auto &timer : timers_) {
                  timer->fire();
                }
              }

            private:
              std::string name_;
              std::unordered_map<std::string, std::string> string_params_;
              std::unordered_map<std::string, int> int_params_;
              std::unordered_map<std::string, bool> bool_params_;
              std::unordered_map<std::string, std::shared_ptr<void>> publishers_;
              std::vector<std::string> subscriptions_;
              std::vector<TimerBase::SharedPtr> timers_;
            };

            inline void init(int, char **) {}
            inline void shutdown() {}
            template<typename NodeT>
            inline void spin(std::shared_ptr<NodeT>) {}

            }  // namespace rclcpp

            #define RCLCPP_INFO(logger, ...)

            #endif
            """
        ),
        encoding="utf-8",
    )

    (stub_root / "geometry_msgs" / "msg" / "twist.hpp").write_text(
        textwrap.dedent(
            """
            #ifndef GEOMETRY_MSGS__MSG__TWIST_HPP_
            #define GEOMETRY_MSGS__MSG__TWIST_HPP_
            #include <memory>
            namespace geometry_msgs { namespace msg {
            struct Vector3 { double x{0.0}; double y{0.0}; double z{0.0}; };
            struct Twist {
              using SharedPtr = std::shared_ptr<Twist>;
              Vector3 linear;
              Vector3 angular;
            };
            }}  // namespace geometry_msgs::msg
            #endif
            """
        ),
        encoding="utf-8",
    )
    (stub_root / "nav_msgs" / "msg" / "odometry.hpp").write_text(
        "#ifndef NAV_MSGS__MSG__ODOMETRY_HPP_\n#define NAV_MSGS__MSG__ODOMETRY_HPP_\nnamespace nav_msgs { namespace msg { struct Odometry {}; }}\n#endif\n",
        encoding="utf-8",
    )
    (stub_root / "sensor_msgs" / "msg" / "imu.hpp").write_text(
        "#ifndef SENSOR_MSGS__MSG__IMU_HPP_\n#define SENSOR_MSGS__MSG__IMU_HPP_\nnamespace sensor_msgs { namespace msg { struct Imu {}; }}\n#endif\n",
        encoding="utf-8",
    )
    (stub_root / "std_msgs" / "msg" / "u_int32.hpp").write_text(
        "#ifndef STD_MSGS__MSG__U_INT32_HPP_\n#define STD_MSGS__MSG__U_INT32_HPP_\n#include <cstdint>\nnamespace std_msgs { namespace msg { struct UInt32 { uint32_t data{0U}; }; }}\n#endif\n",
        encoding="utf-8",
    )

    harness_cpp = tmp_path / "bridge_harness.cpp"
    harness_cpp.write_text(
        textwrap.dedent(
            f"""
            #include <memory>
            #define main rscf_link_bridge_hidden_main
            #include "{(REPO_ROOT / 'ros2_ws/src/rscf_link_bridge/src/rscf_link_bridge_node.cpp').as_posix()}"
            #undef main

            int main()
            {{
              auto node = std::make_shared<RscfLinkBridgeNode>();
              auto base = std::static_pointer_cast<rclcpp::Node>(node);
              auto status_pub = base->find_publisher<std_msgs::msg::UInt32>("/rscf/system_status");

              if (status_pub == nullptr) {{
                return 1;
              }}
              if (base->get_string_param("device_path") != "/dev/serial/by-id/usb-ZEPHYR_RSCF_A_CDC_ACM_4843500F00410035-if00") {{
                return 2;
              }}
              if (base->get_int_param("baudrate") != 115200) {{
                return 3;
              }}
              if (!base->get_bool_param("compat_mode")) {{
                return 4;
              }}
              if (!base->has_subscription("/cmd_vel")) {{
                return 5;
              }}

              base->run_timers_once();
              if (status_pub->publish_count != 1U) {{
                return 6;
              }}
              if (status_pub->last_msg.data != 0x7009U) {{
                return 7;
              }}

              return 0;
            }}
            """
        ),
        encoding="utf-8",
    )

    binary = tmp_path / "bridge_harness"
    compile_cmd = [
        "g++",
        "-std=c++17",
        f"-I{stub_root}",
        str(harness_cpp),
        "-o",
        str(binary),
    ]

    subprocess.run(compile_cmd, check=True, cwd=REPO_ROOT)
    subprocess.run([str(binary)], check=True, cwd=REPO_ROOT)
