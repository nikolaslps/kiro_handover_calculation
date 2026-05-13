cmake_minimum_required(VERSION 3.8)
project(kiro_handover_calculation)

# Global flags
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
# find_package(Eigen3 REQUIRED)
find_package(visualization_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(hri_msgs REQUIRED)
find_package(kiro_handover_interfaces REQUIRED)
find_package(lifecycle_msgs REQUIRED)

include_directories(include)

# --- optimal handover calculator node ---
# declare executable
add_executable(optimal_volume_calculator 
  src/optimal_volume_calculator.cpp
  src/body_selector.cpp
  src/arm_fk.cpp
)

# link dependencies
ament_target_dependencies(optimal_volume_calculator
  rclcpp
  std_msgs
  sensor_msgs
  geometry_msgs
  visualization_msgs
  tf2
  tf2_ros
  tf2_geometry_msgs
  hri_msgs
  kiro_handover_interfaces
  lifecycle_msgs
)

# --- volume clustering node ---
# declare executable
add_executable(volume_clustering 
  src/volume_clustering.cpp
)

# link dependencies
ament_target_dependencies(volume_clustering
  rclcpp
  std_msgs
  sensor_msgs
  geometry_msgs
  visualization_msgs
  tf2
  tf2_ros
  tf2_geometry_msgs
  hri_msgs
  kiro_handover_interfaces
)

# --- image relay node ---
# declare executable
add_executable(image_relay_node 
  src/image_relay.cpp
)

# link dependencies
ament_target_dependencies(image_relay_node 
  rclcpp 
  sensor_msgs
  std_msgs
  tf2
)

# target_include_directories(kiro_handover_calculation PRIVATE ${EIGEN3_INCLUDE_DIRS})
# target_link_libraries(kiro_handover_calculation Eigen3::Eigen)

# install targets
install(TARGETS
  optimal_volume_calculator
  volume_clustering
  image_relay_node
  DESTINATION lib/${PROJECT_NAME}
)

# install public headers
install(DIRECTORY include/
  DESTINATION include/
)

# install launch files
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}/
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
