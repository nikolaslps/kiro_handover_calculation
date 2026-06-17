# ROS2 Launch Arguments

Customize the behavior of the calculation pipeline using the parameters exposed through the main launch file.

## 1. Coordinate Frames
These parameters define the spatial context of the Robot-to-Human tool handover.

| Argument | Default Value | Description |
| :--- | :--- | :--- |
| `planning_frame` | `kiro_base_link` | The robot's reference frame. Clustered handover points are transformed into this frame for MoveIt2 motion planning. |
| `camera_frame` | `camera_color_optical_frame` | The frame used to calculate human selection priority (e.g., who is closest to the sensor) if needed. |

## 2. Camera Topics (Relay Configuration)
These arguments define the source topics that the **Image Relay** node will listen to. The relay node only publishes these inputs to global handover topics when the handover phase is active.

| Argument | Default Value | Description |
| :--- | :--- | :--- |
| `color_img_topic` | `/camera/camera/color/image_raw/compressed` | Input color image stream (Compressed). |
| `color_info_topic` | `/camera/camera/color/camera_info` | Input color camera metadata. |
| `depth_img_topic` | `/camera/camera/aligned_depth_to_color/image_raw/compressedDepth` | Input depth image stream (CompressedDepth). |
| `depth_info_topic` | `/camera/camera/aligned_depth_to_color/camera_info` | Input depth camera metadata. |

## 3. HRI Body Detection Module
These parameters control the lifecycle and data processing of the `hri_body_detect` module.

| Argument | Default Value | Description |
| :--- | :--- | :--- |
| `use_depth` | `true` | Whether to use depth info for 3D skeleton estimation. |
| `image_compressed` | `true` | Set to true to consume compressed camera streams. |
| `auto_configure` | `false` | If true, the HRI module initializes to the 'Inactive' state automatically. |
| `auto_activate` | `false` | If true, the HRI module starts detecting humans immediately upon launch. |