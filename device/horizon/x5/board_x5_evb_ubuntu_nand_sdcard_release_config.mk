#!/bin/bash

export HR_TARGET_VENDOR="horizon"
export HR_TARGET_CHIP="x5"
export HR_SECURE_CHIP=y
export HR_TARGET_BIT="64"
export HR_TARGET_MODE="release"

# 板级名称，对应每一个新的硬件型号
export HR_BOARD_TYPE="soc"
export HR_MEDIUM_TYPE="nand"

# 编译out目录
export HR_BUILD_OUTPUT_DIR=${HR_TOP_DIR}/out
# 编译中间文件输出目录，如uboot、kernel、hbre的编译目录
export HR_TARGET_BUILD_DIR=${HR_BUILD_OUTPUT_DIR}/build
# 产出镜像输出路径，本目录下的镜像文件用于烧录和发布
export HR_TARGET_PRODUCT_DIR=${HR_BUILD_OUTPUT_DIR}/product
# 从build到product的中间产物，如内核、设备树、根文件系统的原始目录和文件
export HR_TARGET_DEPLOY_DIR=${HR_BUILD_OUTPUT_DIR}/deploy
# 编译日志保存目录
export HR_BUILD_LOG_DIR=${HR_BUILD_OUTPUT_DIR}/build_log

# 板级配置文件存放目录
export HR_BOARD_CONF_DIR=${HR_TOP_DIR}/"device/horizon/${HR_TARGET_CHIP}/board_cfg/${HR_BOARD_TYPE}"

# 配置交叉编译工具链
export ARCH="arm64"
export TOOLCHAIN_PATH=/opt/gcc-arm-11.2-2022.02-x86_64-aarch64-none-linux-gnu
export CROSS_COMPILE=${TOOLCHAIN_PATH}/bin/aarch64-none-linux-gnu-

# 使能ccache，加速编译
export HR_CCACHE_SUPPORT=y
export CCACHE_COMMAND="ccache"
export HR_APPEND_CXX_OPTIONS="-DCMAKE_CXX_COMPILER=\"${CCACHE_COMMAND}\" -DCMAKE_CXX_COMPILER_ARG1=\"${CROSS_COMPILE}g++\" -DCMAKE_C_COMPILER=\"${CCACHE_COMMAND}\" -DCMAKE_C_COMPILER_ARG1=\"${CROSS_COMPILE}gcc\""
export HR_CCACHE_DIR="$HOME/.ccache"

# 构建系统常用的工具软件存放路径，比如fiptool
export HR_BUILD_TOOL_PATH=${HR_TOP_DIR}/build/tools
# 分区表，mbr，gpt内容处理的工具软件存放路径
export HR_PARTITION_TOOL_PATH=${HR_TOP_DIR}/build/tools/partition_tools
# avbtools工具脚本存放路径，在对kernel和分区文件系统添加校验信息时需要使用到（mk_system.sh）
export HR_AVB_TOOLS_PATH=${HR_TOP_DIR}/build/tools/android_tools/avbtools
export HR_BD_IMG_TOOLS_PATH=${HR_TOP_DIR}/build/tools/android_tools/build_image

# 分区表配置文件所在目录和文件名
export HR_PART_CONF_FILENAME=${HR_BOARD_CONF_DIR}/x5-evb-ubuntu-nand-gpt.json
export BLK_SZ=512
export NAND_ERASE_SIZE=262144
export NAND_PAGE_SIZE=4096
export NAND_SIZE=$((128 * 1024 * 1024))

# uboot 编译配置文件
export HR_UBOOT_CONFIG_FILE=hobot_x5_evb_nand_sd_defconfig
export HR_ARCH_UBOOT="arm"
# 指定uboot源码的输出目录，如果不设置，则在源码目录下编译
export HR_UBOOT_OUTPUT_DIR=${HR_TARGET_BUILD_DIR}/uboot

# 环境变量配置标志，用来标识当前env环境已经完成了板级配置项的设置
export HR_IS_BOARD_CONFIG_EXPORT="true"
