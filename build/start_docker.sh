#!/bin/bash

set -eu

# 默认值
CONTAINER_IMAGE="dr_xbuild/ubuntu20.04:1.0"
DEFAULT_USER=$(whoami)
DEFAULT_PASSWORD="123456"
DEFAULT_CONTAINER_NAME="dr_xbuild"

# 显示帮助信息
show_help() {
	echo "Usage: $0 [-u USER] [-p PASSWORD] [-i IMAGE] [-w WORKDIR] [-n CONTAINER_NAME] [-h]"
	echo
	echo "Description:"
	echo "This script starts a Docker container with specified configurations."
	echo
	echo "Options:"
	echo "  -u USER      Specify the new username. Default: $DEFAULT_USER"
	echo "  -p PASSWORD  Specify the password for the new user. Default: $DEFAULT_PASSWORD"
	echo "  -i IMAGE     Specify the Docker image version. Default: $CONTAINER_IMAGE"
	echo "  -w WORKDIR   Specify the container's working directory. Default: current directory"
	echo "  -n CONTAINER_NAME  Specify the container name. Default: $DEFAULT_CONTAINER_NAME"
	echo "  -h           Display this help message."
	exit 1
}

# 处理用户选项
while getopts "u:p:i:w:n:h" opt; do
	case $opt in
	u)
		USER_OPTION="$OPTARG"
		;;
	p)
		PASSWORD_OPTION="$OPTARG"
		;;
	i)
		CONTAINER_IMAGE="$OPTARG"
		;;
	w)
		CONTAINER_WORKDIR_OPTION="$OPTARG"
		;;
	n)
		CONTAINER_NAME_OPTION="$OPTARG"
		;;
	h)
		show_help
		;;
	\?)
		echo "Invalid option: -$OPTARG" >&2
		show_help
		;;
	esac
done

# 获取主机用户的UID和GID
HOST_UID=$(id -u)
HOST_GID=$(id -g)

# 获取主机用户名
HOST_USERNAME=$(id -un)

# 设置容器名称
CONTAINER_NAME=${CONTAINER_NAME_OPTION:-$DEFAULT_CONTAINER_NAME}

# 设置容器工作目录
CONTAINER_WORKDIR=$(readlink -f "${CONTAINER_WORKDIR_OPTION:-$(pwd)}")

# 启动 Docker 容器
if [ -z "$(docker ps --filter name="${CONTAINER_NAME}" -q)" ]; then
	docker run \
		--privileged \
		--cap-add=ALL \
		-v /dev:/dev \
		--security-opt=apparmor:unconfined \
		-v /etc/passwd:/etc/passwd:ro -v /etc/group:/etc/group:ro \
		-v /dev:/tmp/dev:ro \
		-v /etc/localtime:/etc/localtime:ro \
		-v "${CONTAINER_WORKDIR}":"${CONTAINER_WORKDIR}" \
		-v "$HOME"/.ccache:"$HOME"/.ccache \
		-w "$CONTAINER_WORKDIR" \
		-e CONTAINER_OWNER_UID="$HOST_UID" \
		-e CONTAINER_OWNER_USERNAME="$HOST_USERNAME" \
		-u "$HOST_UID":"$HOST_GID" \
		--name "$CONTAINER_NAME" -dt \
		"$CONTAINER_IMAGE" /bin/bash

	echo "Starting Docker container $CONTAINER_NAME ..."

	# 设置用户和密码
	USER_TO_SET=${USER_OPTION:-$DEFAULT_USER}
	PASSWORD_TO_SET=${PASSWORD_OPTION:-$DEFAULT_PASSWORD}

	# 给 Docker 中主机用户名添加 sudo 权限和密码
	# 注意：这里使用root用户进入容器
	echo "Setting user and password ..."
	docker exec -it -u 0:0 "$CONTAINER_NAME" /bin/bash -c "echo '$USER_TO_SET ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers && echo '$USER_TO_SET:$PASSWORD_TO_SET' | chpasswd"

	# 安装交叉编译工具链
	echo "Installing cross-compilation toolchain ..."
	docker exec -it -u 0:0 "$CONTAINER_NAME" /bin/bash -c "tar -xf toolchain/arm-gnu-toolchain-11.3.rel1-x86_64-aarch64-none-linux-gnu.tar.xz -C /opt/"
fi

# 进入 Docker 容器
docker exec -it "$CONTAINER_NAME" /bin/bash -i
