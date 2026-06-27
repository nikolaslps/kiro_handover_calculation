# Interface Documentation

This page lists all exposed topics, and services provided by the `kiro_handover_calculation` core workspace package.

## 1. ROS 2 Vulcanexus Node

### Node: `optimal_volume_calculator`
The primary operational worker node responsible for coordinate calculations and human selection.

* **Subscribed Topics:**
    * `/humans/bodies/tracked` (`hri_msgs/msg/IdsList`) —  Monitors the active tracking IDs currently seen by the visual perception system.
    * `/humans/bodies/<body_id>/skeleton2d` (`hri_msgs/msg/Skeleton2D`) — Dynamic subscription reading raw 2D coordinate landmarks from upstream cameras.
    * `/humans/bodies/<active_body_id_>/joint_states` (`sensor_msgs/msg/JointState`) — Dynamic subscription capturing skeletal frame scales to map individual arm lengths.
* **Published Topics:**
    * `/handover_status` (`std_msgs/msg/Bool`) — Notifies the global Lifecycle Manager whether the tracking pipeline is active.
    * `/humans/bodies/<body_id>/handover_volume` (`visualization_msgs/msg/MarkerArray`) — Publishes the collection of points outlining the valid ergonomic interaction workspace.
* **Service Servers:**
    * `/activate_handover_calc` (`kiro_handover_interfaces/srv/ActivateHandover`) — Toggles the internal pipeline calculations and triggers upstream HRI node states.
    * `/get_active_body_id` (`kiro_handover_interfaces/srv/GetActiveBodyID`) — Exposes the current target human identifier to downstream execution nodes.
* **Service Client:**
    * `/hri_body_detect/change_state` (`lifecycle_msgs/srv/ChangeState`) — Issues lifecycle configuration changes (Configure, Activate, Deactivate) to sensors.



### Node: `image_relay_node`
A protective gateway utility designed to conserve internal bus bandwidth before the handover phase.

* **Subscribed Topics:**
    * `/handover_status` (`std_msgs/msg/Bool`) — The master control topic that updates the internal `activate_calc_` state machine flag to handle conditional forwarding.
    * **Dynamic Parameter Input Channels:**
        * Color Image Stream (via `color_img_topic` parameter): Defaults to `/camera/camera/color/image_raw/compressed` (`sensor_msgs/msg/CompressedImage` or `sensor_msgs/msg/Image`).
        * Color Camera Info (via `color_info_topic` parameter): Defaults to `/camera/camera/color/camera_info` (`sensor_msgs/msg/CameraInfo`).
        * Depth Image Stream (via `depth_img_topic` parameter): Defaults to `/camera/camera/aligned_depth_to_color/image_raw/compressedDepth` (`sensor_msgs/msg/CompressedImage` or `sensor_msgs/msg/Image`).
        * Depth Camera Info (via `depth_info_topic` parameter): Defaults to `/camera/camera/aligned_depth_to_color/camera_info` (`sensor_msgs/msg/CameraInfo`).

* **Published Topics (Conditional Output):**
    * `/camera_info` (`sensor_msgs/msg/CameraInfo`) — Forwarded color metadata stream.
    * **If `image_compressed` is `true`:**
        * `/image/compressed` (`sensor_msgs/msg/CompressedImage`) — Gated transport color frames.
        * `/depth_image/compressed` (`sensor_msgs/msg/CompressedImage`) — Gated depth stream (Only if `use_depth` is enabled).
    * **If `image_compressed` is `false`:**
        * `/image` (`sensor_msgs/msg/Image`) — Gated raw color pixel frames.
        * `/depth_image` (`sensor_msgs/msg/Image`) — Gated raw depth frames (Only if `use_depth` is enabled).
    * `/depth_info` (`sensor_msgs/msg/CameraInfo`) — Forwarded depth metadata stream (Only if `use_depth` is enabled).

* **Node Parameters:**
    * `use_depth` (bool, default: `true`): Toggles parsing, subscription creation, and initialization pipelines for the depth-sensing camera topics.
    * `image_compressed` (bool, default: `true`): Directs the node to initialize subscribers/publishers for transport compressed message frameworks (`sensor_msgs/msg/CompressedImage`) rather than heavy uncompressed raw layout messages (`sensor_msgs/msg/Image`).
    * `color_img_topic` (string, default: `"/camera/camera/color/image_raw/compressed"`): Target path identifier for the primary workspace RGB input feed.
    * `color_info_topic` (string, default: `"/camera/camera/color/camera_info"`): Target path identifier for the RGB camera configuration array.
    * `depth_img_topic` (string, default: `"/camera/camera/aligned_depth_to_color/image_raw/compressedDepth"`): Target path identifier for the co-registered depth input feed.
    * `depth_info_topic` (string, default: `"/camera/camera/aligned_depth_to_color/camera_info"`): Target path identifier for the depth camera configuration array.

### Node: `volume_clustering`
* **Service Server:**
    * `/cluster_handover_volume` (`kiro_handover_interfaces/srv/ClusterHandoverVolume`) — Processes continuous spatial point samples into a discrete array of $k$ distinct coordinates optimized for path planners.

> [!NOTE]
> **KIRO Interfaces:** For more information regarding the exact service definitions and structural interface parameters, please refer to the corresponding package repository:[`kiro_handover_interfaces`](https://github.com/nikolaslps/kiro_handover_interfaces).

---

## 2. ROS4HRI & ROS4RI Topic Mapping
The following table details how this module interfaces with the standard ROS4HRI topic structure to ensure robust data interoperability:

| Standard HRI Topic Target | Module Usage Type | Functional Role |
| :--- | :--- | :--- |
| `/humans/bodies/tracked` | **Subscriber** | Ingested by `body_selector` to monitor the human-receiver during occlusions or newly assigned IDs. |
| `/humans/bodies/<body_id>/skeleton2d` | **Subscriber** | Feeds upper/lower arm landmark coordinates to the Forward Kinematics solver (`arm_fk`). |
| `/humans/bodies/<body_id>/joint_states` | **Subscriber** | Triggers structural calculations via `callbackJointStates` to sample `tf2` transforms. Extracted transforms for `l_y_shoulder`, `l_elbow`, `r_y_shoulder`, and `r_elbow` are used to compute geometric upper arm lengths. |
| `/humans/bodies/<body_id>/handover_volume` | **Publisher** | Marker array visualization for RViz of the optimal handover poses. These points are utilized for the optimal handover point selection used by volume_clustering node. |