import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument, ExecuteProcess, RegisterEventHandler, TimerAction
from launch.substitutions import LaunchConfiguration
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node

def generate_launch_description():
    hri_launch_dir = os.path.join(get_package_share_directory('hri_body_detect'), 'launch')

    # Declare launch arguments for the HRI body detection module
    use_depth_arg = DeclareLaunchArgument(
        'use_depth', 
        default_value='true'
    )
    
    image_compressed_arg = DeclareLaunchArgument(
        'image_compressed', 
        default_value='true'
    )

    hri_module_configure_arg = DeclareLaunchArgument(
        'auto_configure', 
        default_value='false'
    )

    hri_module_activate_arg = DeclareLaunchArgument(
        'auto_activate', 
        default_value='false'
    )

    # Planning frame argument for your MoveIt2 setup
    planning_frame_arg = DeclareLaunchArgument(
        'planning_frame', 
        default_value='kiro_base_link'
    )

    # Image topic arguments based on your camera setup
    color_img_topic_arg = DeclareLaunchArgument(
        'color_img_topic',
        default_value='/camera/camera/color/image_raw/compressed'
    )

    color_info_topic_arg = DeclareLaunchArgument(
        'color_info_topic',
        default_value='/camera/camera/color/camera_info'
    )

    depth_img_topic_arg = DeclareLaunchArgument(
        'depth_img_topic',
        default_value='/camera/camera/aligned_depth_to_color/image_raw/compressedDepth'
    )

    depth_info_topic_arg = DeclareLaunchArgument(
        'depth_info_topic',
        default_value='/camera/camera/aligned_depth_to_color/camera_info'
    )

    camera_frame_arg = DeclareLaunchArgument(
        'camera_frame', 
        default_value='camera_color_optical_frame'
    )

    # --- HRI Body Detection Module ---
    hri_detect = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(hri_launch_dir, 'hri_body_detect.launch.py')),
        launch_arguments={
            'use_depth': LaunchConfiguration('use_depth'),                  # if true, use depth image for body detection
            'image_compressed': LaunchConfiguration('image_compressed'),    # if true, use compressed image
            'auto_configure': LaunchConfiguration('auto_configure'),        # if true, HRI body detect module gets Confugured
            'auto_activate': LaunchConfiguration('auto_activate')           # if true, HRI body detect module gets Active
        }.items()
    )

    # --- Optimal Handover Calculator ---
    optimal_vol_calculator = Node(
        package='kiro_handover_calculation',
        executable='optimal_volume_calculator',
        name='optimal_volume_calculator',
        output='screen',
        parameters=[{
            'camera_frame': LaunchConfiguration('camera_frame')
        }]
    )

    # --- Volume Clustering Service ---
    volume_clustering = Node(
        package='kiro_handover_calculation',
        executable='volume_clustering',
        name='volume_clustering',
        output='screen',
        parameters=[{
            'planning_frame': LaunchConfiguration('planning_frame')
        }]
    )

    # --- Image Relay ---
    image_relay_node = Node(
        package='kiro_handover_calculation',
        executable='image_relay_node',
        name='image_relay_node',
        parameters=[{
            'use_depth': LaunchConfiguration('use_depth'),
            'image_compressed': LaunchConfiguration('image_compressed'),
            'color_img_topic': LaunchConfiguration('color_img_topic'),
            'color_info_topic': LaunchConfiguration('color_info_topic'),
            'depth_img_topic': LaunchConfiguration('depth_img_topic'),
            'depth_info_topic': LaunchConfiguration('depth_info_topic')
        }]
    )

    return LaunchDescription([
        use_depth_arg,
        image_compressed_arg,
        hri_module_configure_arg,
        hri_module_activate_arg,
        planning_frame_arg,
        color_img_topic_arg,
        color_info_topic_arg,
        depth_img_topic_arg,
        depth_info_topic_arg,
        camera_frame_arg,
        hri_detect,
        image_relay_node,
        volume_clustering,
        optimal_vol_calculator
    ])