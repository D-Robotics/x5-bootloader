#!/bin/bash

set -e

################### setting utils_funcs ###################
SCRIPT_DIR="$( cd "$( dirname "$(readlink -f "${BASH_SOURCE[0]}")" )" && pwd )"
source "$SCRIPT_DIR/utils_funcs.sh"

HR_TOP_DIR=$(realpath "${SCRIPT_DIR}"/../)
export HR_TOP_DIR
export HR_LOCAL_DIR=${SCRIPT_DIR}

# 选择板级配置
# 如果无参数则打印出板级配置列表，输入数字选择配置
# 如果输入 数字 参数，则使用该数字代表的板级配置
# 如果直接输入 板级配置名， 则直接使用该板级配置
if [ "$1" = "lunch" ];then
	lunch_board_combo "${@:2}"
	exit 0
fi

# check board config
check_board_config "${@:1}"

# print all configs and set ccache
build_env_setup

export BUILD_OUTPUT_DIR=${HR_BUILD_OUTPUT_DIR}
mkdir -p ${BUILD_OUTPUT_DIR} ${HR_TARGET_BUILD_DIR} ${HR_TARGET_PRODUCT_DIR} ${HR_TARGET_DEPLOY_DIR}

# Redirect both stdout and stderr to log files
[ ! -z ${HR_BUILD_LOG_DIR} ] && [ ! -d ${HR_BUILD_LOG_DIR} ] && mkdir ${HR_BUILD_LOG_DIR}
log_file=${HR_BUILD_LOG_DIR}/build_$(date +"%Y%m%d_%H%M%S").log
exec > >(tee -a ${log_file}) 2>&1

# release board config
if [ -L ${HR_TOP_DIR}/device/.board_config.mk ] && [ ! -z ${HR_TARGET_PRODUCT_DIR} ]; then
	cp ${HR_TOP_DIR}/device/.board_config.mk ${HR_TARGET_PRODUCT_DIR}/board_config.mk
fi

function help_msg
{
	script_name=$(basename "$0")
	echo -e "=================================================================================="
	echo -e "   \\  //   Welcome to the D-Robotics xbuild system!"
	echo -e "    \\//    Working directory: ${HR_TOP_DIR}"
	echo -e "    //\\                                             "
	echo -e "   //  \\                                            "
	echo -e "=================================================================================="
	echo "Available commands for $script_name:"
	echo "./$script_name [all | function] [module] [clean | distclean]"
	echo "Support functions :"
	echo -e "\thelp ${avail_func[*]}"
	echo "Usage example:"
	echo -e "\t./$script_name all"
	echo -e "\t./$script_name miniboot [clean | distclean]"
	echo -e "\t./$script_name uboot [clean | distclean]"
	echo -e "\t./$script_name boot [clean | distclean]"
	echo -e "\t./$script_name boot module [clean]  -- eg: ./$script_name boot spi"
	echo -e "\t./$script_name system [clean | distclean]"
	echo -e "\t./$script_name hbre [help | all ] [modules] [pack | clean | distclean]"
	echo -e "\t./$script_name hbre module [pack | clean | distclean] -- eg: ./$script_name hbre liblog"
	echo -e "\t./$script_name app [help | all ] [modules] [pack | clean | distclean]"
	echo -e "\t./$script_name app module [pack | clean | distclean]"
	echo -e "\t./$script_name clean"
	echo -e "\t./$script_name distclean"
	echo -e "\t./$script_name uboot menuconfig"
	echo -e "\t./$script_name boot menuconfig"
	echo -e "\t./$script_name help"
	echo -e "=================================================================================="
	echo -e "After executing 'source build/quickcmd.sh'"
	echo -e "The following Shortcut commands can be used:"
	source "${HR_LOCAL_DIR}"/quickcmd.sh > /dev/null 2>&1
	shortcuts_help
	echo -e "=================================================================================="
	exit 0
}

function build_component {
	component_name=$1
	script_path="${*:2}"

	echo "**********************************************************************"
	echo "[INFO]: Starting the build process for $component_name"
	bash $script_path || exit 1
	echo "[INFO]: Completed the build for $component_name"
	echo "**********************************************************************"
}

function build_miniboot {
	 build_component "miniboot" "${HR_LOCAL_DIR}/mk_miniboot.sh" "$@"
}

function build_uboot {
	build_component "uboot" "${HR_LOCAL_DIR}/mk_uboot.sh" "$@"
}

function build_factory
{
	build_component "uart_usb" "${HR_LOCAL_DIR}/pack_uart_usb.sh" "$@"
}

function build_boot {
	build_component "boot" "${HR_LOCAL_DIR}/mk_boot.sh" "$@"
}

function build_system
{
	build_component "system" "${HR_LOCAL_DIR}/mk_system.sh" "$@"
}

function build_hbre
{
	if [ "${HR_MEDIUM_TYPE}" = "nor" ]; then
		return
	fi
	build_component "hbre" "${HR_LOCAL_DIR}/mk_hbre.sh" "$@"
}

function build_app
{
	# tmp code
	fs_type=$(get_part_attr system fs_type)
	if [ "${fs_type}" = "ubifs" ]; then
		return
	fi
	build_component "app" "${HR_LOCAL_DIR}/mk_app.sh" "$@"
}

function truncate_fill_image
{
	part_size=$(get_part_attr "${1}" "size")
	part_type=$(get_part_attr "${1}" "part_type")
	part_medium=$(get_part_attr "${1}" "medium")
	part_fs=$(get_part_attr "${1}" "fs_type")
	local image_path="${2}"
	local img_name="${3}"
	local flash_suffix_img=${HR_TARGET_PRODUCT_DIR}/flash_suffix.img
	SYSTEM_BUILD_DIR=${HR_TARGET_DEPLOY_DIR}/${HR_SYSTEM_PART_NAME}

	# FIXME: If there is actual data in the partition behind the mirror, pack will be skipped.
	# 先固定跳过log和userdata分区，要优化成根据配置来找到最后一个有数据的分区
	case "${part_name}" in
		log*|userdata*)
			rm -f "${HR_TARGET_PRODUCT_DIR}/${part_name}".img
			if [ "${part_medium}" == "emmc" ]; then
				echo "[INFO]: Skip pack partition: ${part_name}"
				return
			fi
			;;
	esac

	if [ "${part_fs}" = "ubifs" ] && [ ! -f "${image_path}" ]; then
		if [ ! -d "${SYSTEM_BUILD_DIR}/${part_name}" ];then
			mkdir -p "${SYSTEM_BUILD_DIR}/${part_name}"
		fi
		"${HR_PARTITION_TOOL_PATH}"/mk_ubifs.sh "${part_name}" "${SYSTEM_BUILD_DIR}/${part_name}"
	fi

	# Ensure the image exists for non-PERM part_type
	if [ "${part_type}" != "PERMANENT" ] && [ ! -f "${image_path}" ]; then
		echo "[ERROR]: ${image_path} does not exist"
		exit 1
	fi

	if [ "${part_type}" != "PERMANENT" ] && [ "${part_name}" != "userdata" ]; then
		# Ensure part_size is not smaller than the image size
		if [ "${part_size}" -lt "$(stat -c "%s" "${image_path}")" ]; then
			echo "[ERROR]: part_size(${part_size}) of ${1} is smaller than the size of ${image_path}"
			exit 1
		fi
	fi

	# Truncate image to part_size
	if [ "${part_medium}" = "nand" ] || [ "${part_medium}" = "nor" ]; then
		if [ ! -f "${image_path}" ];then
			rel_size=0
		else
			rel_size=$(du -b "${image_path}" | awk '{print $1}')
		fi
		append_size=$((${part_size} - ${rel_size}))
		if [ "${append_size}" -gt "0"  ];then
			dd if=/dev/zero bs=1 count="${append_size}" | tr '\000' '\377' > "${flash_suffix_img}"
			if [ ! -f "${image_path}" ];then
				cat "${flash_suffix_img}" > "${image_path}.bak"
			else
				cat "${image_path}" "${flash_suffix_img}" > "${image_path}.bak"
			fi
			mv "${image_path}.bak" "${image_path}"
			rm "${flash_suffix_img}"
		fi
	else
		truncate -s "${part_size}" "${image_path}"
	fi

	# Append image content to disk.img
	echo "[INFO]: cat ${image_path} >> ${img_name}"
	cat "${image_path}" >> "${img_name}"
}

function build_pack
{
	local emmc_raw_img=${HR_TARGET_PRODUCT_DIR}/emmc_disk.img
	local emmc_sparse_img=${HR_TARGET_PRODUCT_DIR}/emmc_disk.simg
	local flash_raw_img=${HR_TARGET_PRODUCT_DIR}/flash_disk.img
	local nand_raw_img=${HR_TARGET_PRODUCT_DIR}/nand_disk.img
	local nor_raw_img=${HR_TARGET_PRODUCT_DIR}/nor_disk.img


	if [ "$1" = "all" ]; then
		#"${HR_PARTITION_TOOL_PATH}"/pack_avb_img.sh vbmeta vbmeta

		echo "**********************************************************************"
		echo "[INFO]: Starting pack all image to *_disk.img"
		cd "${HR_LOCAL_DIR}"

		rm -f "${emmc_raw_img}" "${emmc_sparse_img}" "${flash_raw_img}" \
				"${nand_raw_img}" "${nor_raw_img}"

		rm -f "${HR_TARGET_PRODUCT_DIR}"/*-gpt.json

		part_names=$(get_part_name_list)

		medium=$(get_part_attr miniboot medium)
		if [ "${medium}" = "emmc" ];then
			echo "[INFO]: cat ${HR_TARGET_PRODUCT_DIR}/miniboot_all.img >> ${emmc_raw_img}"
			cat "${HR_TARGET_PRODUCT_DIR}/miniboot_all.img" >> "${emmc_raw_img}"
		elif [ "${medium}" = "nand" ] || [ "${medium}" = "nor" ];then
			echo "[INFO]: cat ${HR_TARGET_PRODUCT_DIR}/miniboot_all.img >> ${flash_raw_img}"
			cat "${HR_TARGET_PRODUCT_DIR}/miniboot_all.img" >> "${flash_raw_img}"
		fi

		found_uboot=false
		for part_name in ${part_names};do
			# uboot分区之前的gpt mbr...等分区内容已经包含在miniboot中，跳过pack直到uboot分区
			case "${part_name}" in
				uboot*)
					found_uboot=true
					;;
				*)
					if [ "${found_uboot}" = "false" ]; then
						continue
					fi
					;;
			esac
			medium=$(get_part_attr "${part_name}" medium)
			if [ "${part_name}" = "boot" ] || [ "${part_name}" = "boot_a" ] ||
			 [ "${part_name}" = "ubootenv" ]; then
				if [ "${medium}" = "nand" ] || [ "${medium}" = "nor" ];then
					"${HR_PARTITION_TOOL_PATH}"/mk_ubifs.sh "${part_name}" ""
				fi
			fi
			if [ "${found_uboot}" = "true" ];then
				if [ "${medium}" = "emmc" ];then
					truncate_fill_image "${part_name}" "${HR_TARGET_PRODUCT_DIR}/${part_name//_*}.img" "${emmc_raw_img}"
				elif [ "${medium}" = "nand" ] || [ "${medium}" = "nor" ];then
					truncate_fill_image "${part_name}" "${HR_TARGET_PRODUCT_DIR}/${part_name//_*}.img" "${flash_raw_img}"
				fi
			fi
		done

		if [ -f "${emmc_raw_img}" ];then
			echo "[INFO]: End pack all image to ${emmc_raw_img}"
			echo "[INFO]: Make the raw image into a sparse image: ${emmc_sparse_img}"
			img2simg "${emmc_raw_img}" "${emmc_sparse_img}"
		fi

		if [ "${medium}" = "nand" ];then
			mv -v "${flash_raw_img}" "${nand_raw_img}"
		elif [ "${medium}" = "nor" ];then
			mv -v "${flash_raw_img}" "${nor_raw_img}"
		fi

		echo "**********************************************************************"
	elif [ "$1" = "clean" ] || [ "$1" = "distclean" ];then
		echo "[INFO]: Clean all image"
		# rm -f ${BUILD_OUTPUT_DIR}/${image_name}
		rm -f "${HR_TARGET_PRODUCT_DIR}"/*.img
		rm -f "${HR_TARGET_PRODUCT_DIR}"/*-gpt.json
		if [ -d "${HR_TARGET_DEPLOY_DIR}/vbmeta" ]; then
			rm -rf "${HR_TARGET_DEPLOY_DIR}/vbmeta"
		fi

		if [ "$1" = "distclean" ]; then
			if [ -d "${BUILD_OUTPUT_DIR}" ]; then
				rm -rf "${BUILD_OUTPUT_DIR}"
			fi
		fi
	fi
	return 0
}

function build_otapackage()
{
	local deploy_otapack_dir=${HR_TARGET_DEPLOY_DIR}/ota_packages
	local src_ota_tool_dir=${HR_BUILD_TOOL_PATH}/ota_tools
	local product_ota_dir=${HR_TARGET_PRODUCT_DIR}/ota_packages

	print_help() {
		echo "Usage:"
		echo "    ./bd.sh otapackage"
		echo "        create all_in_one ota packages"
		echo "    ./bd.sh otapackage --help"
		echo "        help information"
	}

    echo "==========Start begin otapackage==========="
	case "$1" in
		"all")
			echo "create all_in_one.zip "
			${src_ota_tool_dir}/mk_otapackage.py sys_pkg \
				--partition_file "${HR_TARGET_PRODUCT_DIR}/${HR_PART_CONF_FILENAME##*/}" \
				--ota_process "${deploy_otapack_dir}"/tools/ota_process \
				--image_dir "${HR_TARGET_PRODUCT_DIR}/" \
				--prepare_dir "${deploy_otapack_dir}/" \
				--sign_key "${src_ota_tool_dir}"/keys/private_key.pem \
				--out_dir "${product_ota_dir}/"
			;;
		"help")
			print_help
			exit 0
			;;
		*)
			echo "Unknown cmd: $1"
			print_help
			exit 1
			;;
	esac
}

function build_all
{
	opt=$1

	build_miniboot "$*"

	build_uboot "$opt"

	build_factory "$opt"

	build_pack "$opt"
}

avail_func=("all" "lunch" "miniboot" "uboot" "factory" "boot" "hbre" "system" "app" "pack" "otapackage")

if [ $# -eq 0 ];then
	build_all all
elif [ $# -eq 1 ];then
	if [ "$1" = "clean" ];then
		build_all clean
	elif [ "$1" = "distclean" ];then
		build_all distclean
		rm -f "${HR_TOP_DIR}"/device/.board_config.mk
	elif [ "$1" = "no_secure" ];then
		build_all all no_secure
	elif [ "$1" = "all" ];then
		build_all all
	elif inList "$1" "${avail_func[*]}";then
		build_"$1" all
	else
		help_msg
	fi
else
	if inList "$1" "${avail_func[*]}";then
		build_"$1" "${@:2}"
	else
		help_msg
	fi
fi
echo "**********************************************************************"
echo "[INFO]: Congratulations, the build succeeded."
echo "**********************************************************************"
