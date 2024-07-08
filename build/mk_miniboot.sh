#!/bin/bash

# set -x
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

export MINIBOOT_SOURCE_DIR="${HR_TOP_DIR}"/miniboot
export MINIBOOT_TARGET_DEPLOY_DIR=${HR_TARGET_DEPLOY_DIR}/miniboot
[ "${MINIBOOT_TARGET_DEPLOY_DIR}" != "/miniboot" ] && [ ! -d "${MINIBOOT_TARGET_DEPLOY_DIR}" ] && mkdir -p "${MINIBOOT_TARGET_DEPLOY_DIR}"

export BL3_TARGET_DEPLOY_DIR=${MINIBOOT_TARGET_DEPLOY_DIR}/bl3

function mk_gpt
{
	${HR_PARTITION_TOOL_PATH}/gen_gpt.py \
		${HR_PART_CONF_FILENAME} \
		${HR_TARGET_PRODUCT_DIR}
}

function mk_mbr
{
	${HR_PARTITION_TOOL_PATH}/genMbr.py make_mbr \
		--partition_file ${HR_PART_CONF_FILENAME} \
		--chip ${HR_TARGET_CHIP} \
		--output ${HR_TARGET_PRODUCT_DIR}
}

function mk_nor_cfg
{
	local nor_cfg="${HR_TARGET_PRODUCT_DIR}/norcfg.img"
	{
		# magic: 0x4E4F524D
		echo -n -e "\x4D\x52\x4F\x4E"

		# status_qe_reg/qe_index/status_reg_bits
		# IS25WP512 NOR Flash cfg
		echo -n -e ${HR_NOR_CFG}

		# extra parameters for nand flash
		echo -n -e "\x00\x00\x00\x00"
	} > ${nor_cfg}
}

function mk_bl2
{
	if [ -n "${NO_SECURE}" ] && [ "${NO_SECURE}" = "y" ];then
		cp "${MINIBOOT_SOURCE_DIR}/bl2/ns/bl2_fip_ns.bin" "${MINIBOOT_TARGET_DEPLOY_DIR}/bl2.img"
		cp "${MINIBOOT_SOURCE_DIR}/bl2/ns/bl2_uart_fip_ns.bin" "${MINIBOOT_TARGET_DEPLOY_DIR}/bl2_uart.img"
		cp "${MINIBOOT_SOURCE_DIR}/bl2/ns/bl2_usb2_fip_ns.bin" "${MINIBOOT_TARGET_DEPLOY_DIR}/bl2_usb2.img"
		cp "${MINIBOOT_SOURCE_DIR}/bl2/ns/bl2_usb3_fip_ns.bin" "${MINIBOOT_TARGET_DEPLOY_DIR}/bl2_usb3.img"
	else
		cp "${MINIBOOT_SOURCE_DIR}/bl2/bl2_fip.bin" "${MINIBOOT_TARGET_DEPLOY_DIR}/bl2.img"
		cp "${MINIBOOT_SOURCE_DIR}/bl2/bl2_uart_fip.bin" "${MINIBOOT_TARGET_DEPLOY_DIR}/bl2_uart.img"
		cp "${MINIBOOT_SOURCE_DIR}/bl2/bl2_usb2_fip.bin" "${MINIBOOT_TARGET_DEPLOY_DIR}/bl2_usb2.img"
		cp "${MINIBOOT_SOURCE_DIR}/bl2/bl2_usb3_fip.bin" "${MINIBOOT_TARGET_DEPLOY_DIR}/bl2_usb3.img"
	fi
}

function pack_bl3x
{
	fip_tool=${HR_BUILD_TOOL_PATH}/fiptool

	if [ ! -f "${fip_tool}" ]; then
		echo "[ERROE]: ${fip_tool} is not exists"
		exit 1
	fi

	${fip_tool} create \
		--soc-fw-cert ${BL3_TARGET_DEPLOY_DIR}/soc_fw_content.crt \
		--soc-fw-key-cert ${BL3_TARGET_DEPLOY_DIR}/soc_fw_key.crt \
		--tos-fw-cert ${BL3_TARGET_DEPLOY_DIR}/tos_fw_content.crt \
		--tos-fw-key-cert ${BL3_TARGET_DEPLOY_DIR}/tos_fw_key.crt \
		--tos-fw ${BL3_TARGET_DEPLOY_DIR}/tee-header_v2.bin\
		--tos-fw-extra1 ${BL3_TARGET_DEPLOY_DIR}/tee-pager_v2.bin\
		--tos-fw-extra2 ${BL3_TARGET_DEPLOY_DIR}/tee-pageable_v2.bin\
		--soc-fw ${BL3_TARGET_DEPLOY_DIR}/bl31.bin \
		--ddr-fw ${BL3_TARGET_DEPLOY_DIR}/bl2_ddr.bin  \
		--ddr-fw-key-cert ${BL3_TARGET_DEPLOY_DIR}/bl2_ddr_key.cert  \
		--ddr-fw-cert ${BL3_TARGET_DEPLOY_DIR}/bl2_ddr.cert  \
		"${MINIBOOT_TARGET_DEPLOY_DIR}"/bl3x.img || {
			echo "[ERROR]: ${fip_tool} bl3x.img package failed"
			exit 1
		}
}

function mk_bl3x
{
	if [ ! -d "${BL3_TARGET_DEPLOY_DIR}" ]; then
		mkdir -p "${BL3_TARGET_DEPLOY_DIR}"
	fi
	cpfiles "${MINIBOOT_SOURCE_DIR}/bl3x/bl31.bin" "${BL3_TARGET_DEPLOY_DIR}/"
	cpfiles "${MINIBOOT_SOURCE_DIR}/bl3x/*.crt" ${BL3_TARGET_DEPLOY_DIR}/
	cpfiles "${MINIBOOT_SOURCE_DIR}/bl3x/tee-*" ${BL3_TARGET_DEPLOY_DIR}/
	cpfiles "${MINIBOOT_SOURCE_DIR}/bl3x/bl2_ddr.bin" ${BL3_TARGET_DEPLOY_DIR}/
	cpfiles "${MINIBOOT_SOURCE_DIR}/bl3x/bl2_ddr.cert" ${BL3_TARGET_DEPLOY_DIR}/
	cpfiles "${MINIBOOT_SOURCE_DIR}/bl3x/bl2_ddr_key.cert" ${BL3_TARGET_DEPLOY_DIR}/

	pack_bl3x
}

function truncate_fill_image
{
	part_size=$(get_part_attr "${1}" "size")

	local image_path="${2}"
	local out_img="${3}"

	# Truncate image to part_size
	truncate -s "${part_size}" "${image_path}"

	echo "[INFO]: ${1}: ${image_path} >> ${out_img}"
	cat "${image_path}" >> "${out_img}"
}

function pack_miniboot
{
	out_img=${HR_TARGET_PRODUCT_DIR}/miniboot.img
	echo "[INFO]: Pack Miniboot(bl2,bl3x)..."
	rm -f "${out_img}"

	part_names=$(get_miniboot_list)

	for part_name in ${part_names};do
		truncate_fill_image "${part_name}" ${MINIBOOT_TARGET_DEPLOY_DIR}/${part_name//_*}.img ${out_img}
	done
}

function pack_miniboot_all
{
	miniboot_all_img=${HR_TARGET_PRODUCT_DIR}/miniboot_all.img

	echo "[INFO]: Pack Miniboot(gpt,mbr,miniboot,misc)..."
	rm -f ${miniboot_all_img}

	part_names=$(get_part_name_list)

	for part_name in ${part_names};do
		# 把misc分区之前的gpt mbr...等分区内容打包进miniboot分区中
		truncate_fill_image "${part_name}" ${HR_TARGET_PRODUCT_DIR}/${part_name//_*}.img ${miniboot_all_img}
		case "${part_name}" in
			misc)
				break
				;;
		esac
	done
}

function mk_ta
{
	cd "${MINIBOOT_SOURCE_DIR}/optee/hobot_tee_devkit/"
	./build.sh
	cd -
}

function build_all
{
	mk_gpt
	if [ "${HR_MEDIUM_TYPE}" = "nor" ]; then
		mk_nor_cfg
	fi
	mk_mbr
	mk_bl2
	mk_bl3x
	mk_ta
	pack_miniboot
	pack_miniboot_all
}

function build_clean
{
	echo "[INFO]: Clean miniboot"
	if [ -d "${BL3_TARGET_DEPLOY_DIR}" ]; then
		rm -rf "${BL3_TARGET_DEPLOY_DIR}"
	fi
	if [ -d "${MINIBOOT_TARGET_DEPLOY_DIR}" ]; then
		rm -rf "${MINIBOOT_TARGET_DEPLOY_DIR}"
	fi
	if [ -f "${HR_TARGET_PRODUCT_DIR}/miniboot.img" ]; then
		rm -rf "${HR_TARGET_PRODUCT_DIR}/miniboot.img"
	fi

	rm -f "${HR_TARGET_PRODUCT_DIR}"/{gpt.img,mbr.img,bl2.img,bl3x.img}

	cd "${MINIBOOT_SOURCE_DIR}/optee/hobot_tee_devkit/"
	./build.sh clean
	cd -
}

if [ $# -eq 0 ]; then
	build_all
elif [ $# -eq 1 ]; then
	if [ "$1" = "all" ]; then
		build_all
	elif [ "$1" = "clean" ] || [ "$1" = "distclean" ]; then
		build_clean
	elif [ "$1" = "no_secure" ]; then
		NO_SECURE=y
		build_all
	fi
elif [ $# -eq 2 ]; then
	if [ "$1" = "no_secure" ] || [ "$2" = "no_secure" ]; then
		NO_SECURE=y
		build_all
	fi
fi
