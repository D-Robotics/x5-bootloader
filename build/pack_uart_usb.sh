#!/bin/bash

# When unexpected situations occur during script execution, exit immediately to avoid errors being ignored and incorrect final results
set -e

################### setting utils_funcs ###################
SCRIPT_DIR="$( cd "$( dirname "$(readlink -f "${BASH_SOURCE[0]}")" )" && pwd )"
source "$SCRIPT_DIR/utils_funcs.sh"

HR_TOP_DIR=$(realpath "${SCRIPT_DIR}"/../)
export HR_TOP_DIR
export HR_LOCAL_DIR=${SCRIPT_DIR}

# check board config
check_board_config "${@:1}"

# 编译出来的镜像保存位置
mkdir -p "${HR_TARGET_BUILD_DIR}" "${HR_TARGET_PRODUCT_DIR}" "${HR_TARGET_DEPLOY_DIR}"

function pack_uart_usb_img()
{
	# two images for uart/usb
	# image 1: bl2 fip (256k align) + other fip(ddr + bl2 cfg)
	# image 2: all bl3x, bl31 + bl32 + bl33
	mkdir -p ${uart_usb_dir}
	cp ${bl2_out_dir}/bl2.img ${uart_usb_dir}/bl2_ddr.bin
	truncate -s 256K ${uart_usb_dir}/bl2_ddr.bin
	${fip_tool} create \
		--ddr-fw "${bl3x_out_dir}"/bl2_ddr.bin  \
		--ddr-fw-key-cert "${bl3x_out_dir}"/bl2_ddr_key.cert  \
		--ddr-fw-cert "${bl3x_out_dir}"/bl2_ddr.cert  \
		--trusted-key-cert "${uboot_out_dir}"/trusted_key.crt \
		--nt-fw-cert "${uboot_out_dir}"/nt_fw_content.crt \
		--nt-fw-key-cert "${uboot_out_dir}"/nt_fw_key.crt \
		--hb-bl2-cfg "${uboot_out_dir}"/bl2_cfg.bin \
		"${uart_usb_dir}"/ddr.bin || {
			echo "[ERROE]: ddr.bin package failed"
			exit 1
		}
	cat ${uart_usb_dir}/ddr.bin >>${uart_usb_dir}/bl2_ddr.bin
	rm ${uart_usb_dir}/ddr.bin

	cp ${bl2_out_dir}/bl2_uart.img ${uart_usb_dir}/bl2_uart_ddr.bin
	truncate -s 256K ${uart_usb_dir}/bl2_uart_ddr.bin
	${fip_tool} create \
		--ddr-fw "${bl3x_out_dir}"/bl2_ddr.bin  \
		--ddr-fw-key-cert "${bl3x_out_dir}"/bl2_ddr_key.cert  \
		--ddr-fw-cert "${bl3x_out_dir}"/bl2_ddr.cert  \
		--trusted-key-cert "${uboot_out_dir}"/trusted_key.crt \
		--nt-fw-cert "${uboot_out_dir}"/nt_fw_content.crt \
		--nt-fw-key-cert "${uboot_out_dir}"/nt_fw_key.crt \
		--hb-bl2-cfg "${uboot_out_dir}"/bl2_cfg.bin \
		"${uart_usb_dir}"/ddr.bin || {
			echo "[ERROE]: ddr.bin package failed"
			exit 1
		}
	cat ${uart_usb_dir}/ddr.bin >>${uart_usb_dir}/bl2_uart_ddr.bin
	rm ${uart_usb_dir}/ddr.bin

	${fip_tool} create \
		--soc-fw-cert "${bl3x_out_dir}"/soc_fw_content.crt \
		--soc-fw-key-cert "${bl3x_out_dir}"/soc_fw_key.crt \
		--tos-fw-cert "${bl3x_out_dir}"/tos_fw_content.crt \
		--tos-fw-key-cert "${bl3x_out_dir}"/tos_fw_key.crt \
		--tos-fw "${bl3x_out_dir}"/tee-header_v2.bin\
		--tos-fw-extra1 "${bl3x_out_dir}"/tee-pager_v2.bin\
		--tos-fw-extra2 "${bl3x_out_dir}"/tee-pageable_v2.bin\
		--soc-fw "${bl3x_out_dir}"/bl31.bin \
		--trusted-key-cert "${uboot_out_dir}"/trusted_key.crt \
		--nt-fw-cert "${uboot_out_dir}"/nt_fw_content.crt \
		--nt-fw-key-cert "${uboot_out_dir}"/nt_fw_key.crt \
		--nt-fw "${uboot_out_dir}"/u-boot.bin \
		"${uart_usb_dir}"/bl3x_all.bin || {
			echo "[ERROE]: bl3x_all.bin package failed"
			exit 1
		}
}

function build_all()
{
	echo "[INFO]: Generate the bl2 and bl3x_all images required for flashing tools"
	fip_tool=${HR_BUILD_TOOL_PATH}/fiptool
	bl2_out_dir=${HR_TARGET_DEPLOY_DIR}/miniboot
	bl3x_out_dir=${HR_TARGET_DEPLOY_DIR}/miniboot/bl3
	uart_usb_dir=${HR_TARGET_PRODUCT_DIR}/uart_usb
	uboot_out_dir=${HR_TARGET_DEPLOY_DIR}/uboot

	if [ ! -f "${fip_tool}" ]; then
		echo "[ERROE]: ${fip_tool} is not exists"
		exit 1
	fi

	if ! [ -d "${HR_TARGET_DEPLOY_DIR}/miniboot" ] && ! [ -f "${uboot_out_dir}/u-boot.bin" ]; then
		echo "[ERROR]: Please compile miniboot and uboot first."
		exit 1
	fi

	pack_uart_usb_img
}

function build_clean()
{
	echo "[INFO]: Clean uart_usb images"
	if [ -d "${HR_TARGET_PRODUCT_DIR}/uart_usb" ]; then
		rm -rf ${HR_TARGET_PRODUCT_DIR}/uart_usb
	fi
}

# 根据命令参数编译
if [ $# -eq 0 ] || [ "$1" = "all" ]; then
	build_all
elif [ "$1" = "clean" ] || [ "$1" = "distclean" ]; then
	build_clean
fi