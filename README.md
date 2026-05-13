# ARISE KIRO -- Handover Calculation Node

![Vulcanexus](https://img.shields.io/badge/Vulcanexus-Humble-00214c?style=for-the-badge&logo=ros)
![License](https://img.shields.io/badge/license-Apache%202.0-green?style=for-the-badge&logo=apache)
![Build Status](https://img.shields.io/badge/build-passing-brightgreen?style=for-the-badge&logo=github-actions&logoColor=white)
![Docker](https://img.shields.io/badge/Docker-Ready-2496ED?style=for-the-badge&logo=docker&logoColor=white)

## Table of Contents

- [Overview](#overview)
- [File Structure](#file-structure)
- [Launch Arguments](#launch-arguments)
- [Installation](#installation)
- [Usage](#usage)
- [Supported Distribution](#supported-distribution)
- [License](#license)


## Overview

The `kiro_handover_calculation` node provides a robust computational framework for determining the optimal handover points between a robot and a human. The system leverages the **ROS4HRI** `hri_body_detect` module for tracking and extracting the skeleton of a human, alongside supplemental modules such as `hri_msgs` and `human_description`. 

Our custom package `kiro_handover_calculation` uses data provided by the `hri_body_detect` module -specifically active body IDs (tracked humans) and skeletal landmarks- to compute the **Optimal Handover Volume** for close-proximity Human-Robot Interaction (HRI) tasks.

The node performs the following internal calculations:
1.  **Kinematic Extraction:** Computes the Forward Kinematics (FK) for the tracked human's upper and lower arm.
2.  **Ergonomic Scores:** Extracts a handover volume based on optimized **REBA/RULA** scores (Rapid Entire Body Assessment / Rapid Upper Limb Assessment), which represents the optimal handover positions.
3.  **Visualization:** The resulting volume is published as a visualization marker in RViz under the topic:
    `/humans/bodies/<body_id>/handover_volume`

To ensure performance and modularity, the node uses an interface, `kiro_handover_interfaces`, which exposes three primary services:
* `ActivateHandover`: Activates the Handover pipeline.
* `GetActiveBodyID`: Returns the ID of the human currently prioritized for interaction.
* `ClusterHandoverVolume`: Processes the raw volume into $k$ discrete clusters for motion planning.

For more information about the interfaces, please refer to the [Handover Interfaces](https://github.com/nikolaslps/kiro_handover_interfaces) repo.

## File Structure
```text
kiro_handover_calculation/
├── include/
│   └── kiro_handover_calculation/
│           ├── arm_fk.hpp                      # Forward Kinematics logic to extract the handover volume
│           ├── body_selector.hpp               # Human prioritization logic
│           ├── image_relay.hpp                 # Gated camera topic forwarding
│           ├── optimal_volume_calculator.hpp   # Main handover node
│           └── volume_clustering.hpp           # K-Means clustering service
├── launch/
│   └── kiro_handover_calculation.launch.py     # Main system launch file
├── src/
│   ├── arm_fk.cpp
│   ├── body_selector.cpp
│   ├── image_relay.cpp
│   ├── optimal_volume_calculator.cpp
│   └── volume_clustering.cpp
├── CMakeLists.txt                              # Build configuration
├── package.xml                                 # Package metadata and dependencies
├── requirements.txt                            # Python dependencies if built from source
├── README.md                                   # Documentation
└── LICENSE                                     # License information
```

## Launch Arguments
### 1. Coordinate Frames
These parameters define the spatial context of the handover.

| Argument | Default Value | Description |
| :--- | :--- | :--- |
| `planning_frame` | `kiro_base_link` | The robot's reference frame. Clustered handover points are transformed into this frame for MoveIt2 motion planning. |
| `camera_frame` | `camera_color_optical_frame` | The frame used to calculate human selection priority (e.g., who is closest to the sensor) if needed. |

### 2. Camera Topics (Relay Configuration)
These define the source topics that the **Image Relay** node will listen to. The relay only publishes to global handover topics when the handover phase is active.

| Argument | Default Value | Description |
| :--- | :--- | :--- |
| `color_img_topic` | `/camera/camera/color/image_raw/compressed` | Input color image stream (Compressed). |
| `color_info_topic` | `/camera/camera/color/camera_info` | Input color camera metadata. |
| `depth_img_topic` | `/camera/camera/aligned_depth_to_color/image_raw/compressedDepth` | Input depth image stream (CompressedDepth). |
| `depth_info_topic` | `/camera/camera/aligned_depth_to_color/camera_info` | Input depth camera metadata. |

### 3. HRI Body Detection Module
Controls the lifecycle and data processing of the `hri_body_detect` module.

| Argument | Default Value | Description |
| :--- | :--- | :--- |
| `use_depth` | `true` | Whether to use depth info for 3D skeleton estimation. |
| `image_compressed` | `true` | Set to true to consume compressed camera streams. |
| `auto_configure` | `false` | If true, the HRI module initializes to the 'Inactive' state automatically. |
| `auto_activate` | `false` | If true, the HRI module starts detecting humans immediately upon launch. |

## Installation

### 1. Docker Container (Recommended)
For the most stable experience, we recommend using our pre-configured Docker environment.
* Refer to the [ARISE KIRO Docker Repository](https://github.com/andvatistas/ARISE-KIRO-reusable-modules) for setup assistance.
* Follow the provided `README.md` within that repository to pull the image and start the container.

### 2. Building from Source
**Note**: Building from source has not been fully tested in all environments. We strongly recommend using the Docker version above.

#### Prerequisites
Ensure you are running **ROS 2 Humble**, preferably on the **Vulcanexus** image.

#### Setup Workspace
Clone the repositories into your ROS 2 workspace `src` folder:

```bash
cd ~/ros2_ws/src
```

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

```bash
# Handover Calculation Node
git clone https://github.com/nikolaslps/kiro_handover_calculation.git
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

In order to trigger the hri body detection and handover calculation to start, run:
```bash
ros2 service call /activate_handover_calc kiro_handover_interfaces/srv/ActivateHandover "{handover_phase: true}"
```

## Supported Distribution
* **Dockerized ROS 2 Humble on the Vulcanexus image**

## License
This project is licensed under the **Apache License 2.0**. See the [LICENSE](LICENSE) file for details.


