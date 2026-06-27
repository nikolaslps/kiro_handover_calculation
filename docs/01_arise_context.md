# ARISE Ecosystem Context & Core Integration

This document outlines how the `kiro_handover_calculation` module interoperates with the core components of the ARISE All-in-one Middleware ecosystem, ensuring compliance with standardization protocols and cross-platform reusability.

## 1. Vulcanexus & ROS 2 Alignment
The module is natively built and verified on the **ROS 2 Vulcanexus (Humble)** ecosystem, leveraging advanced middleware layer features to guarantee deterministic behavior in close-proximity collaboration:

* **eProsima Fast DDS Optimization & RMW Configuration:** The tracking pipeline employs tailored Quality of Service (QoS) profiles to manage data-dense streams. To fully exploit eProsima's high-performance throughput, the integrated container setup directly targets the fast intermediate middleware implementation. The repository's workspace automation script inside the `/docker` directory explicitly forces this binding by setting the underlying environment flag:
  ```bash
  --env RMW_IMPLEMENTATION=rmw_fastrtps_cpp
  ```
* **Centralized Discovery Service Configuration:** To eliminate heavy network multicast traffic overhead common on shared industrial plant networks, the system is designed to support eProsima's **Discovery Server** paradigm, by uncommenting the following encironmental variable inside the `/docker/run_kiro_hri_calc.bash` script. 
  ```bash
  --env ROS_DISCOVERY_SERVER=10.0.17.100:11811 # setup yours
  ```
* **Component-Based Containerization Ready:** All processing nodes (`optimal_volume_calculator`, `image_relay`, and `volume_clustering`) are developed as modular ROS 2 Components which enables further reusability and development.

## 2. ROS4HRI & ROS4RI Standardization Compliance
To ensure this package functions as a plug-and-play asset across different ARISE application fields, it fully adopts the official **ROS4HRI (Human-Robot Interaction)** standard:

* **Ecosystem-Wide Compatibility:** The nodes subscribe to the standard tracking arrays (`/humans/bodies/tracked`) to monitor workspace occupancy, alongside tracking metadata channels (`/humans/bodies/<body_id>/joint_states` and `/humans/bodies/<body_id>/skeleton2d`). Because it conforms to this layout, the calculation core can be combined out-of-the-box with any ROS4HRI-compliant perception module without requiring internal code refactoring.

### 2.1 Community Contribution
Our commitment to the ROS4HRI ecosystem extends beyond implementation to active maintenance and collaborative improvement. We have contributed directly to the core perception pipeline inside the `hri_body_detect` module to ensure robust handling of compressed sensor data:

* **Accepted Upstream Contribution:** Extended the `hri_body_detect` module to safely and natively decode `compressedDepth` image formats. By aligning the implementation with the official`image_transport_plugins` standard, we introduced a precise parsing mechanism for the 12-byte `ConfigHeader`. This fix prevents `AttributeError` failures when using compressed image types and ensures reliable data ingestion for `16UC1` and `32FC1` depth formats, which is critical for real-time perception in the ARISE demonstrator.
* **Pull Request Reference:** [ROS4HRI hri_body_detect Pull Request #2](https://github.com/ros4hri/hri_body_detect/pull/2)
