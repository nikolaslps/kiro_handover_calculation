# Role in the TRL 6-7 Demonstrator

This module serves as the primary situational-awareness component for calculating handover points within the ARISE-KIRO Agile Assembly and Handover Demonstrator. In a TRL6-7 industrial workstation environment, it operates continuously during the active handover phase to ensure a safe, ergonomic, and seamless exchange of tools between human workers and the platform manipulator by translating raw visual skeleton data into optimal handover points for object transfer.


## 1 Structural Runtime Execution Flow
The diagram below illustrates the overall flow of actions and high-level states during the execution of the KIRO collaborative task. The `kiro_handover_calculaiton` is activated during the `deliver_tool` phase (indicated as step 5 in the diagram), specifically during `hand-off` operation to the human worker (as opposed to delivery to a workbench).

<table>
  <tr>
    <td align="center"><img src="../media/system_flowchart.png" alt="Internal Software Architecture Data Exchange Flows" width="1000"/></td>
  </tr>
  <tr>
    <td align="center"><b>Figure 1:</b> Diagram of the robot’s workflow.</td>
  </tr>
</table>

### 1.2 System Architectural Diagram
The functional block diagram below maps the continuous runtime data flow across the primary system nodes, detailing specific ROS 2 topics and service request boundaries. This subsystem is fully active during the handover phase, mapping directly to the initial internal calculations after the **"Approach hand off position"** state shown in the workflow diagram above.

```mermaid
graph TD
    %% Define Subgraphs to distinguish external stacks from this package
    subgraph Perception ["Perception and Human-Tracking"]
        A[Intel RealSense D455 Camera] -->|Raw RGB-D Streams| B[hri_body_detect Node]
    end

    subgraph Calculation ["kiro_handover_calculation (This Module)"]
        B -->|/humans/bodies/tracked<br>hri_msgs/TrackedBodies| C[body_selector Component]
        B -->|/humans/bodies/ID/joint_states<br>sensor_msgs/JointState tf2| D[arm_fk Solver]
        
        C -->|Prioritized Body ID String Lock| D
        D -->|Upper-Limb Kinematics Matrix| E[optimal_volume_calculator Node]
        
        E -->|/humans/bodies/ID/handover_volume<br>visualization_msgs/MarkerArray| F[volume_clustering Service Node]
        E -->|/humans/bodies/ID/handover_volume<br>visualization_msgs/MarkerArray| RViz[RViz 2 Visualization]
    end

    subgraph Execution ["kiro_handover_execution "]
        G[handover_execution Node] -->|cluster_handover_volume service request<br>srv/ClusterHandoverVolume Request| F
        F -->|Array of Centroids<br>srv/ClusterHandoverVolume Response| G
        G -->|Safe Path Joint Trajectories| H[MoveIt2 Interface]
    end

    %% Interactive Node Group Styling
    style Calculation fill:#f5faff,stroke:#0055aa,stroke-width:2px
    style C fill:#ffffff,stroke:#333333,stroke-width:1px
    style D fill:#ffffff,stroke:#333333,stroke-width:1px
    style E fill:#ffffff,stroke:#0055aa,stroke-width:2px
    style F fill:#ffffff,stroke:#333333,stroke-width:1px
    style RViz fill:#fff7e6,stroke:#d46b08,stroke-width:1px
```

### 1.3 System Sequence Diagram
The following sequence diagram details the initialization and data processing flow when a handover is requested.

```mermaid
sequenceDiagram
    participant Orchestrator as handover_execution Node
    participant Calculator as optimal_volume_calculator Node
    participant BodyDetect as hri_body_detect Node
    participant Clustering as cluster_handover_volume Service

    Orchestrator->>Calculator: /activate_handover_calc (Start)
    Calculator->>BodyDetect: Configure & Activate Lifecycle
    BodyDetect-->>Calculator: Success
    
    loop Tracking Loop
        BodyDetect->>Calculator: Publish /humans/bodies/tracked
        Calculator->>Calculator: Select Active Body ID
        Calculator->>BodyDetect: Request Joint States
        Calculator->>Calculator: Compute Forward Kinematics (arm_fk)
        Calculator->>Calculator: Calculate Optimal Volume
        Calculator->>Clustering: Publish MarkerArray
    end

    Orchestrator->>Clustering: /cluster_handover_volume (Request)
    Clustering-->>Orchestrator: Return Clustered Centroids
```

## 2 System Validation 

The `kiro_handover_calculation` framework was validated using a dual-stage testing pipeline: first under high-fidelity simulation using Gazebo environment to verify forward kinematics consistency and clustering convergence limits, and subsequently on physical hardware to establish real-world reliability.

After successfully installing and running this package (and its dependencies), the output in RViz will look like the following Figure.

<table>
  <tr>
    <td align="center"><img src="../media/hri_rviz_volume.png" alt="Robot-Human Interaction" width="400"/></td>
  </tr>
  <tr>
    <td align="center"><b>Figure 2:</b> Geometric visualization of the ergonomic workspace volume (green MarkerArray point cloud) and tracking coordinate frames inside RViz 2.</td>
  </tr>
</table>