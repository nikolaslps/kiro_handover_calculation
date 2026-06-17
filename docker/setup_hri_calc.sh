#!/bin/bash

CONTAINER_NAME="kiro_hri_calc_vulcanexus"
WS_NAME="hri_calc_ws"
SRC_NAME="src"

echo -e "\e[36m======================================================\e[0m"
echo -e "\e[36m      KIRO HRI CALCULATIONS: Master Setup System      \e[0m"
echo -e "\e[36m======================================================\e[0m"

REPO_ROOT="$(pwd)"
if [ ! -f "package.xml" ] || [ ! -d "docker" ]; then
    echo -e "\e[31mError: Please run this script from the repository root directory!\e[0m"
    echo -e "e.g., ./docker/setup_hri_calc.sh"
    exit 1
fi

echo -e "\e[33m[1/3] Creating local shared workspace '$WS_NAME'...\e[0m"
mkdir -p "$WS_NAME/$SRC_NAME"

echo -e "\e[33m[2/4] Copying local package to workspace...\e[0m"
if [ -d "$WS_NAME/$SRC_NAME/kiro_handover_calculation" ]; then
    echo -e "\e[32m  -> Local package already copied. Refreshing source...\e[0m"
    rm -rf "$WS_NAME/$SRC_NAME/kiro_handover_calculation"
fi
mkdir -p "$WS_NAME/$SRC_NAME/kiro_handover_calculation"
rsync -av . "$WS_NAME/$SRC_NAME/kiro_handover_calculation" --exclude="$WS_NAME" --exclude=".git" > /dev/null


echo -e "\e[33m[2/3] Cloning repositories to host ...\e[0m"
cd "$WS_NAME/$SRC_NAME"

# List of repos
repos=(
    "https://github.com/ros4hri/human_description.git"
    "https://github.com/ros4hri/hri_msgs.git"
    "https://github.com/ros4hri/hri_rviz.git"
    "https://github.com/ros4hri/libhri.git"
    "https://github.com/pal-robotics/launch_pal.git"
)

for repo in "${repos[@]}"; do
    repo_name=$(basename "$repo" .git)
    if [ -d "$repo_name" ]; then
        echo -e "\e[32m  -> $repo_name already exists. Skipping clone.\e[0m"
    else
        git clone "$repo"
    fi
done

if [ ! -d "hri_body_detect" ]; then 
    echo -e "\e[33m  -> Cloning hri_body_detect (feature-kiro-integration)...\e[0m"
    git clone -b feature-kiro-integration https://github.com/nikolaslps/hri_body_detect.git
fi

if [ ! -d "kiro_handover_interfaces" ]; then 
    echo -e "\e[33m  -> Cloning kiro_handover_interfaces...\e[0m"
    git clone https://github.com/nikolaslps/kiro_handover_interfaces
fi

cd "$REPO_ROOT"

# Build the Docker Image
echo -e "\e[33m[3/3] Building the Docker image '$CONTAINER_NAME'...\e[0m"
if [ -f "docker/Dockerfile" ]; then
    docker build -t "$CONTAINER_NAME" -f docker/Dockerfile docker/
else
    echo -e "\e[31mError: Dockerfile not found in docker/ directory!\e[0m"
    exit 1
fi

echo -e "\e[33m[4/4] Building ROS 2 Workspace inside the container...\e[0m"

docker run --rm \
    --volume "$(pwd)/hri_calc_ws:/root/hri_calc_ws:rw" \
    $CONTAINER_NAME \
    bash -c "source /opt/vulcanexus/humble/setup.bash && \
            cd /root/hri_calc_ws && \
            rosdep update && \
            rosdep install --from-paths src --ignore-src -y -r \
            --skip-keys \"python3-lap python3-protobuf magic_enum python3-ikpy\" && \
            colcon build --symlink-install"

echo -e "\e[36m================================================\e[0m"
echo -e "\e[32mSUCCESS: Workspace ready and Image built.\e[0m"
echo -e "\e[36mTo run: ./docker/run_kiro_hri_calc.bash\e[0m"
echo -e "\e[36m================================================\e[0m"