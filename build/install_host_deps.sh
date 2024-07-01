#!/bin/bash

set -e

this_user="$(whoami)"
if [ "${this_user}" != "root" ]; then
	echo "[ERROR]: This script requires root privileges. Please execute it with sudo."
	exit 1
fi

# Get Ubuntu system version
ubuntu_version=$(grep -oP 'VERSION_ID="\K[0-9.]+' /etc/os-release | tr -d '"')

echo "Ubuntu version: $ubuntu_version"

general_deps="tzdata tree bc hashdeep kmod file wget curl cpio unzip rsync liblz4-tool jq"
build_deps="build-essential make cmake bison flex ccache zlib1g-dev libssl-dev libncurses-dev u-boot-tools device-tree-compiler cryptsetup-bin"
sparseimg_deps="android-sdk-libsparse-utils"
fatfs_deps="dosfstools mtools"
mtd_deps="mtd-utils"
extfs_deps="e2fsprogs"

all_deps="${general_deps} ${build_deps} \
	${sparseimg_deps} ${fatfs_deps} ${mtd_deps}"

# Check the version and install software dependencies accordingly
if [[ $ubuntu_version == "18.04" ]]; then
	echo "[INFO]: Ubuntu 18.04, installing software dependencies..."
	apt-get install -y ${all_deps} ${extfs_deps}
	apt-get install -y python3 python3-pip
elif [[ $ubuntu_version == "20.04" ]]; then
	echo "[INFO]: Ubuntu 20.04, installing software dependencies..."
	apt-get install -y ${all_deps} ${extfs_deps}
	apt-get install -y python3 python3-pip
elif [[ $ubuntu_version == "22.04" ]]; then
	echo "[INFO]: Ubuntu 22.04, installing software dependencies..."
	apt-get install -y ${all_deps}
	apt-get install -y python3-pip
else
	echo "[ERROR]: Unsupported Ubuntu version: ${ubuntu_version}"
	exit 1
fi

echo "[INFO]: Installation completed!"

