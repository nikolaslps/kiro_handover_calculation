# Installation and Usage Guide

This guide covers setting up the `kiro_handover_calculation` pipeline. You can either deploy using the recommended docker container layout included directly in this repository or build natively from source.

---

## Installation

### 1. Pre-configured Docker Container (Recommended)
For a stable development experience with all system and Python dependencies pre-installed, use the native Docker environment files located inside the `/docker` directory of this repository.

* Refer to the internal [Docker Installation](../docker/Docker-Install.md) for full installation setup steps.


### 2. Building from Source

> [!WARNING]
> Native compilation on the host machine has not been fully verified across all hardware profiles (only tested on Ubuntu 22.04 and partially on Ubuntu 24.04). Using the integrated Docker automation above is highly recommended to prevent dependency drift or configuration conflicts.

> [!IMPORTANT]
> **Prerequisites:** Ensure your system is running **ROS 2 Humble**, preferably deployed on a [Vulcanexus Humble](https://docs.vulcanexus.org/en/latest/) base image.

#### Setup Workspace
Create a standard development workspace and an underlying source directory:

```bash
mkdir -p ~/ros2_ws/src
cd ~/ros2_ws/src
```

> [!IMPORTANT]
> **Repository Placement:** Before proceeding, you must manually copy some files from this repository straight into your new source space.
> 
> Your directory layout **must** look exactly like this for the build system to work:
> ```text
> ~/ros2_ws/src/kiro_handover_calculation/
> ├── include/
> │   └── kiro_handover_calculation/
> │       ├── arm_fk.hpp
> │       ├── body_selector.hpp
> │       ├── image_relay.hpp
> │       ├── optimal_volume_calculator.hpp
> │       └── volume_clustering.hpp
> ├── launch/
> │   └── kiro_handover_calculation.launch.py
> ├── src/
> │   ├── arm_fk.cpp
> │   ├── body_selector.cpp
> │   ├── image_relay.cpp
> │   ├── optimal_volume_calculator.cpp
> │   └── volume_clustering.cpp
> ├── CMakeLists.txt
> ├── package.xml
> └── requirements.txt
> ```

Clone the remaining mandatory external packages into that same `~/ros2_ws/src` folder:

```bash
# Human URDF models & frames
git clone https://github.com/ros4hri/human_description.git
```

```bash
# ROS4HRI standard messages
git clone https://github.com/ros4hri/hri_msgs.git
```

```bash
# ROS4HRI RViz setup
git clone https://github.com/ros4hri/hri_rviz.git
```

```bash
# MediaPipe-based body tracking
git clone -b feature-kiro-integration https://github.com/nikolaslps/hri_body_detect.git
```

```bash
# ROS4HRI helper library
git clone https://github.com/ros4hri/libhri.git
```

```bash
# PAL launch configuration 
git clone https://github.com/pal-robotics/launch_pal.git
```

```bash
# Handover service definitions
git clone https://github.com/nikolaslps/kiro_handover_interfaces
```

Install Dependencies
```bash
cd ~/ros2_ws
sudo rosdep init # May not be necessary
rosdep update
apt-get update
apt-get install -y ros-humble-magic-enum ros-humble-diagnostic-aggregator
rosdep install --from-paths src --ignore-src -y -r --rosdistro humble
wget https://raw.githubusercontent.com/Neargye/magic_enum/master/include/magic_enum/magic_enum.hpp -O /usr/include/magic_enum.hpp
```

Download the python requirements for the `ROS4HRI` module
```bash
cd ~/ros2_ws/src/kiro_handover_calculation/
pip install -r requirements.txt
```

Build the packages by running the following from inside the `ros2_ws`:
```bash
cd ~/ros2_ws
colcon build --symlink-install
source install/setup.bash
```

## Usage 
Launch the handover nodes by running:
```bash
ros2 launch kiro_handover_calculation kiro_handover_calculation.launch.py
```

By default, the perception stack loads in unconfigured state to preserve resources. When the handover phase comes, trigger the calculation node by running the following service call:
```bash
ros2 service call /activate_handover_calc kiro_handover_interfaces/srv/ActivateHandover "{handover_phase: true}"
```