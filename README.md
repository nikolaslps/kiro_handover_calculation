# ARISE -- KIRO Handover Calculation Module

![Vulcanexus](https://img.shields.io/badge/Vulcanexus-Humble-00214c?style=for-the-badge&logo=ros)
![License](https://img.shields.io/badge/license-Apache%202.0-green?style=for-the-badge&logo=apache)
![Build Status](https://img.shields.io/badge/build-manual-lightgrey?style=for-the-badge&logo=github-actions&logoColor=white)
![Docker](https://img.shields.io/badge/Docker-Ready-2496ED?style=for-the-badge&logo=docker&logoColor=white)

## Table of Contents

- [Overview](#overview)
- [Supported Setup](#supported-setup)
- [File Structure](#file-structure)
- [Contact Information](#contact-information)
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


> [!NOTE]
> **KIRO Interfaces:** For more information regarding the exact service definitions and structural interface parameters, please refer to the corresponding package repository:[`kiro_handover_interfaces`](https://github.com/nikolaslps/kiro_handover_interfaces).

## Supported Setup

| Category | Tested On | Expected Compatibility | Not Supported / Unknown |
| :--- | :--- | :--- | :--- |
| **Middleware & OS** | **Vulcanexus Humble** (Ubuntu 22.04 LTS) utilizing **Fast DDS** as the default RMW middleware layer | Standard ROS 2 Humble setups | Older ROS distributions (e.g., Foxy, Galactic) or ROS 1 |
| **Sensors** | **Intel RealSense D455** | Any RGB-D sensor providing synchronized depth/color topics | Monocular 2D webcams (requires depth information) |

> [!NOTE]
>  **Important Middleware Note:** This module is optimized for the **Vulcanexus** ecosystem. It utilizes **Fast DDS** advanced features (such as optimized Quality of Service profiles for high-throughput camera image relays and skeletal landmark arrays) to maintain low-latency tracking in close-proximity human-robot interaction.

## File Structure
```text
kiro_handover_calculation/
├── docker/
│   ├── Docker-Install.md                       # Docker Installation and Launch Manual
│   ├── Dockerfile                              # Dockerfile based on Vulcanexus image
│   ├── run_kiro_hri_calc.bash                  # Script to launch the Docker Container
│   └── setup_hri_calc.sh                       # Script to build the Docker Container
├── docs/
│   ├── 01_arise_context.md                     # ARISE Ecosystem Context & Core Integration
│   ├── 02_interfaces.md                        # Interface Documentation
│   ├── 03_installation.md                      # Installation and Usage Guide
│   ├── 04_launch_ros_nodes.md                  # ROS2 Launch Arguments
│   └── 05_role_in_demonstrator.md              # Role in the TRL 6-7 Demonstrator
├── include/
│   └── kiro_handover_calculation/
│       ├── arm_fk.hpp                          # Forward Kinematics logic to extract the handover volume
│       ├── body_selector.hpp                   # Human prioritization logic
│       ├── image_relay.hpp                     # Gated camera topic forwarding
│       ├── optimal_volume_calculator.hpp       # Main handover node
│       └── volume_clustering.hpp               # K-Means clustering service
├── launch/
│   └── kiro_handover_calculation.launch.py     # Main system launch file
├── media/                                      # Images 
├── src/
│   ├── arm_fk.cpp
│   ├── body_selector.cpp
│   ├── image_relay.cpp
│   ├── optimal_volume_calculator.cpp
│   └── volume_clustering.cpp
├── .gitignore
├── CMakeLists.txt                              # Build configuration
├── LICENSE                                     # License information
├── package.xml                                 # Package metadata and dependencies
├── README.md                                   # Overview of the ARISE KIRO specific package
└── requirements.txt                            # Python dependencies if built from source
```

> [!WARNING]
> **Camera Proximity & Depth Stability:** The baseline 3D coordinate estimation depends heavily on stable depth data. Operators must maintain a moderate distance from the Intel RealSense D455 camera; if an operator stands too close to the sensor, the near-range depth measurements become unstable or imprecise, degrading the accuracy of the upper-limb kinematic and ergonomic volume calculations.

## Contact Information

For queries regarding the development, replication, or integration of this calculation module within the ARISE framework, feel free to reach out:

* **Module Developer:** Nikolaos Lappas
* **GitHub:** [nikolaslps](https://github.com/nikolaslps)
* **Email:** [nikolas.lappas.2003@gmail.com](mailto:nikolas.lappas.2003@gmail.com)

## License
This project is licensed under the **Apache License 2.0**. See the [LICENSE](LICENSE) file for details.


