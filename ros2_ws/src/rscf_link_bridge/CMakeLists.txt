cmake_minimum_required(VERSION 3.8)
project(rscf_link_bridge)

find_package(ament_cmake REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(rscf_link_bridge_node
  src/rscf_link_bridge_node.cpp
)

ament_target_dependencies(rscf_link_bridge_node
  geometry_msgs
  nav_msgs
  rclcpp
  sensor_msgs
  std_msgs
)

install(TARGETS
  rscf_link_bridge_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
