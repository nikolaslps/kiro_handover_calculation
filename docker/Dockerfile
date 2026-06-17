FROM eprosima/vulcanexus:humble-desktop

RUN apt-get update && apt-get install -y \
    python3-pip python3-opencv python3-dev libprotobuf-dev protobuf-compiler ffmpeg libsm6 libxext6 libnlopt-dev libnlopt-cxx-dev\
    git wget curl nano vim \
    ros-humble-diagnostic-aggregator \
    ros-humble-joint-state-publisher-gui \
    ros-humble-robot-state-publisher \
    ros-humble-tf-transformations \
    ros-humble-rqt-image-view \
    ros-humble-usb-cam \
    ros-humble-sensor-msgs \
    ros-humble-tf-transformations \
    ros-humble-topic-tools \
    ros-humble-tf2-geometry-msgs \
    ros-humble-interactive-markers \
    ros-humble-rmw-fastrtps-cpp \
    ros-humble-launch-param-builder \
    ros-humble-urdf-test \
    ros-humble-magic-enum \
    && rm -rf /var/lib/apt/lists/*
ENV RMW_IMPLEMENTATION=rmw_fastrtps_cpp 

RUN pip3 install --no-cache-dir catkin_pkg lark-parser psutil ikpy lap transforms3d pycparser

RUN pip3 install --no-cache-dir mediapipe==0.10.9
RUN pip3 install --no-cache-dir numpy==1.23.5
RUN pip3 install --no-cache-dir opencv-contrib-python==4.10.0.84
RUN pip3 install --no-cache-dir opencv-python
RUN pip3 install --no-cache-dir sounddevice==0.5.0
RUN pip3 install --no-cache-dir protobuf==3.20.3
RUN pip3 install --no-cache-dir CFFI==1.17.1 absl-py==2.1.0 flatbuffers==24.3.25 matplotlib empy==3.3.4
RUN pip3 install --no-cache-dir numpy==1.23.5

WORKDIR /root/hri_calc_ws

RUN echo "source /opt/vulcanexus/humble/setup.bash" >> ~/.bashrc
RUN echo "if [ -f /root/hri_calc_ws/install/setup.bash ]; then source /root/hri_calc_ws/install/setup.bash; fi" >> ~/.bashrc

# Default entrypoint
CMD ["/bin/bash"]