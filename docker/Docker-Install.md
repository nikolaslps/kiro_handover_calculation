# Docker Installation and Launch

This is the recommended way of installing and running the `kiro_handover_calculation` package. 

## Build the Docker Container
Build the docker image by running the executable from the repository root .
```bash
chmod +x ./docker/setup_hri_calc.sh # if not executable already
```
```
./docker/setup_hri_calc.sh
```

If the installation finishes correctly, you will be greeted with a success message:

> [!NOTE]
> **Expected Build Output:**
> ```text
> ================================================
> SUCCESS: Workspace ready and Image built.
> To run: ./docker/run_kiro_hri_calc.bash
> ================================================
> ```

## Start the Docker Container
Run the executable.
```bash
chmod +x ./docker/run_kiro_hri_calc.bash # if not executable already
```
```
./docker/run_kiro_hri_calc.bash
```

> [!NOTE]
> **Expected Terminal Transition:**
> ```text
> non-network local connections being added to access control list
> root@container-environment:~/hri_calc_ws#
> ```

## From inside the container 
1. Source the workspace
```bash
source install/setup.bash # may not be necessary
```

2. The following command launches 
    - hri_body_detect node (starts in an *unconfigured* state)
    - image_relay_node
    - optimal_vol_calculator node
    - volume_clustering node

```bash
ros2 launch kiro_handover_calculation kiro_handover_calculation.launch.py
```

> [!TIP]
> Configuration Check: Before launching, review the default parameters directly inside `kiro_handover_calculation.launch.py`. For a full deep-dive into configurable parameters and launch options, see the [Launch ROS Nodes Documentation](docs/04_launch_ros_nodes.md).


## Testing the services
This package uses the [kiro_handover_interfaces](https://github.com/nikolaslps/kiro_handover_interfaces) package in order to communicate with the mission controller (FSM) which orchestrates the desired tasks and the [kiro_handover_execution](https://github.com/nikolaslps/kiro_handover_execution) package which is responsible for the movement of the robotic arm.

### 1. Get the active body ID
```bash
ros2 service call /get_active_body_id kiro_handover_interfaces/srv/GetActiveBodyID
```

### 2. Get the `k` optimal Poses for the specific `body_id`
```bash
ros2 service call /cluster_handover_volume kiro_handover_interfaces/srv/ClusterHandoverVolume "{k: <number_clusters>, body_id: '<body_id>'}"
```

### 3. Start the Handover pipeline (Stop if `handover_phase: false`)
```bash
ros2 service call /activate_handover_calc kiro_handover_interfaces/srv/ActivateHandover "{handover_phase: true}"
```

#### Verify the Log Output

Upon launching, the system initializes your custom calculation pipeline alongside the standard ROS4HRI tracking backend. Verify a successful startup by checking that your terminal logs reflect the following sequence:

```text
[INFO] [hri_body_detect-1]: process started with pid [819510]
[INFO] [image_relay_node-3]: process started with pid [819514]
[INFO] [volume_clustering-4]: process started with pid [819516]
[INFO] [optimal_volume_calculator-5]: process started with pid [819518]

[optimal_volume_calculator-5] [INFO] [optimal_volume_calculator]: Optimal Volume Calculator initialized. Service: /get_active_body_id
[image_relay_node-3] [INFO] [image_relay_node]: Image Relay Node initialized (Compressed: TRUE, Depth: TRUE)
[volume_clustering-4] [INFO] [volume_clustering]: Clustering Service Node Ready.
[hri_body_detect-1] [INFO] [hri_body_detect]: State: Unconfigured.
```

> [!IMPORTANT]
> **Numpy Version Requirement:** If you encounter compatibility or runtime errors related to `numpy`, ensure you are using the specific verified version by running:
> ```bash
> pip3 install "numpy==1.23.5"
> ```