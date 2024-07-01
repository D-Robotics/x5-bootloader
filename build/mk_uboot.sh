#!/bin/bash

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
mkdir -p ${HR_TARGET_BUILD_DIR} ${HR_TARGET_PRODUCT_DIR} ${HR_TARGET_DEPLOY_DIR}

# 使用CPU核心数量减去2的线程数去编译
N=$(( ($(cat /proc/cpuinfo |grep 'processor'|wc -l)  + 1 ) - 2 ))

uboot_config_file=${HR_UBOOT_CONFIG_FILE}
UBOOT_SRC_DIR=${HR_TOP_DIR}/uboot
HR_UBOOT_DEPLOY_DIR=${HR_TARGET_DEPLOY_DIR}/uboot
mkdir -p ${HR_UBOOT_DEPLOY_DIR}

if [ -z "${HR_UBOOT_OUTPUT_DIR}" ]; then
	BUILD_OPTIONS="ARCH=${HR_ARCH_UBOOT}"
	HR_UBOOT_OUTPUT_DIR=${UBOOT_SRC_DIR}
else
	BUILD_OPTIONS="ARCH=${HR_ARCH_UBOOT} O=${HR_UBOOT_OUTPUT_DIR}"
fi

function gen_bl2_cfg()
{
	python3 "${HR_BUILD_TOOL_PATH}/bl2_cfg.py"  \
		"${HR_BOARD_CONF_DIR}/bl2_cfg/bl2_cfg.json"  \
		"${HR_BOARD_CONF_DIR}/bl2_cfg/bl2_rot_prikey.pem" \
		"${HR_BOARD_CONF_DIR}/bl2_cfg/user_root.key" "${HR_UBOOT_DEPLOY_DIR}"/bl2_cfg.bin || {
		echo "[ERROR]: Generate bl2 config file failed"
		exit 1
	}
}

function uboot_cert()
{
	trusted_key_cert=${HR_UBOOT_DEPLOY_DIR}/trusted_key.crt
	nt_fw_key_cert=${HR_UBOOT_DEPLOY_DIR}/nt_fw_key.crt
	nt_fw_cert=${HR_UBOOT_DEPLOY_DIR}/nt_fw_content.crt
	hb_bl2_cfg=${HR_UBOOT_DEPLOY_DIR}/bl2_cfg.bin
	nt_fw=${HR_UBOOT_DEPLOY_DIR}/u-boot.bin
	bl2_rot_key=${HR_BOARD_CONF_DIR}/bl2_cfg/bl2_rot_prikey.pem
	cert_tool=${HR_BUILD_TOOL_PATH}/cert_create
	if [ ! -f "${cert_tool}" ]; then
		echo "[ERROE]: ${cert_tool} is not exists"
		exit 1
	fi

	${cert_tool}                          \
		-n                                \
		--bl2-rot-key    ${bl2_rot_key}   \
		--tfw-nvctr    0                  \
		--ntfw-nvctr    0                 \
		--key-alg   rsa                   \
		--key-size  4096                  \
		--hash-alg  sha256                \
		--trusted-key-cert  ${trusted_key_cert}      \
		--nt-fw-key-cert ${nt_fw_key_cert}  \
		--nt-fw-cert ${nt_fw_cert}  \
		--nt-fw ${nt_fw}  \
		--hb-bl2-cfg ${hb_bl2_cfg} || {
			echo "[ERROR]: Create uboot certificate failed"
			exit 1
		}
}

function pack_uboot()
{
	fip_tool=${HR_BUILD_TOOL_PATH}/fiptool
	if [ ! -f "${fip_tool}" ]; then
		echo "[ERROE]: ${fip_tool} is not exists"
		exit 1
	fi
	${fip_tool} create \
		--trusted-key-cert "${HR_UBOOT_DEPLOY_DIR}"/trusted_key.crt \
		--nt-fw-cert "${HR_UBOOT_DEPLOY_DIR}"/nt_fw_content.crt \
		--nt-fw-key-cert "${HR_UBOOT_DEPLOY_DIR}"/nt_fw_key.crt \
		--nt-fw "${HR_UBOOT_DEPLOY_DIR}"/u-boot.bin \
		--hb-bl2-cfg "${HR_UBOOT_DEPLOY_DIR}"/bl2_cfg.bin \
		"${HR_TARGET_PRODUCT_DIR}"/uboot.img || {
			echo "[ERROR]: Pack uboot image failed"
			exit 1
		}

	echo "[INFO]: Pack uboot image to ${HR_TARGET_PRODUCT_DIR}/uboot.img"
}

function build_all()
{
	# 配置uboot配置
	echo "[INFO]: uboot defconfig: ${uboot_config_file}"
	make ${BUILD_OPTIONS} ${uboot_config_file} || {
		echo "[ERROE]: make ${uboot_config_file} failed"
		exit 1
	}

	bootmode=$(get_part_attr uboot medium)
	if [ "${bootmode}" = "nand" ] || [ "${bootmode}" = "nor" ];then
		mtdparts_str=$(get_mtd_part_list)
		sed -i "s/CONFIG_MTDPARTS_DEFAULT=.*/CONFIG_MTDPARTS_DEFAULT=\"${mtdparts_str}\"/g" ${HR_UBOOT_OUTPUT_DIR}/.config
	fi

	make ${BUILD_OPTIONS} -j"${N}" || {
		echo "[ERROR]: make uboot failed"
		exit 1
	}

	cp -f ${HR_UBOOT_OUTPUT_DIR}/u-boot.bin ${HR_UBOOT_DEPLOY_DIR}/

	# 制作uboot image
	gen_bl2_cfg
	uboot_cert
	pack_uboot
}

function pack_uboot_full()
{
	out_img=${HR_TARGET_PRODUCT_DIR}/uboot_all.img

	echo "[INFO]: Pack uboot_all image to ${out_img}"
	build_all
	if [ ! -f "${HR_TARGET_PRODUCT_DIR}/miniboot_all.img" ]; then
		echo "Missing miniboot_all.img, please run ./bd.sh miniboot"
		exit 1
	fi
	rm -f "${out_img}"

	echo "[INFO]: uboot_all.img: "${HR_TARGET_PRODUCT_DIR}/miniboot_all.img" > ${out_img}"
	cat "${HR_TARGET_PRODUCT_DIR}/miniboot_all.img" > "${out_img}"
	echo "[INFO]: uboot_all.img: "${HR_TARGET_PRODUCT_DIR}/uboot.img" >> ${out_img}"
	cat "${HR_TARGET_PRODUCT_DIR}/uboot.img" >> "${out_img}"
}

function build_clean()
{
	echo "[INFO]: Clean uboot"
	make ${BUILD_OPTIONS} clean
	rm -f ${HR_TARGET_PRODUCT_DIR}/uboot.img
}

function build_distclean()
{
	echo "[INFO]: Distclean uboot"
	make ${BUILD_OPTIONS} distclean
	rm -f ${HR_TARGET_PRODUCT_DIR}/uboot*.img
}

function uboot_menuconfig() {
	# Check if uboot_config_file variable is set
	if [ -z "${uboot_config_file}" ]; then
		echo "[ERROR]: Uboot defconfig file is not set. Aborting menuconfig."
		return 1
	fi

	# Run menuconfig with the specified U-Boot configuration file
	UBOOT_DEFCONFIG=$(basename "${uboot_config_file}")
	echo "[INFO]: U-Boot menuconfig with ${UBOOT_DEFCONFIG}"
	make ${BUILD_OPTIONS} -C "${UBOOT_SRC_DIR}" "${UBOOT_DEFCONFIG}"

	# 执行 make menuconfig
	script -q -c "make ${BUILD_OPTIONS} -C ${UBOOT_SRC_DIR} menuconfig" /dev/null

	# Check if menuconfig was successful
	if [ $? -eq 0 ]; then
		# Run savedefconfig to save the configuration back to the original file
		make ${BUILD_OPTIONS} -C ${UBOOT_SRC_DIR} savedefconfig
		dest_defconf_path="${HR_TOP_DIR}/uboot/configs/${UBOOT_DEFCONFIG}"
		echo "**** Saving U-Boot defconfig to ${dest_defconf_path} ****"
		cp -f "${HR_UBOOT_OUTPUT_DIR}/defconfig" "${dest_defconf_path}"
	fi

	# Check if savedefconfig was successful
	if [ $? -ne 0 ]; then
		echo "[ERROR]: savedefconfig failed. Configuration may not be saved."
		return 1
	fi

	echo "[INFO]: U-Boot menuconfig completed successfully."
}

# 进入源码目录
cd ${UBOOT_SRC_DIR}

# 根据命令参数编译
if [ $# -eq 0 ] || [ "$1" = "all" ]; then
	build_all
elif [ "$1" = "full" ]; then
	pack_uboot_full
elif [ "$1" = "clean" ]; then
	build_clean
elif [ "$1" = "distclean" ]; then
	build_distclean
elif [ "$1" = "menuconfig" ]; then
	uboot_menuconfig
fi
