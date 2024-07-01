#!/bin/bash
# this file build system.img system_vbmeta.img

# When unexpected situations occur during script execution, exit immediately to avoid errors being ignored and incorrect final results
set -e

################### setting utils_funcs ###################
source "${HR_TOP_DIR}/build/utils_funcs.sh"

function create_config_file()
{
	local file=$1
	rm -rf "${file}"
	touch "${file}"

	part_id=$(get_part_attr "${part_name}" part_num)
	fs_type=$(get_part_attr "${part_name}" fs_type)
	partition_size=$(get_part_attr "${part_name}" size)
	outimage=${HR_TARGET_PRODUCT_DIR}/${part_name}.img
	partition_align_size=$((${partition_size} - 512*1024))
	if [ "${part_name}" == "system" ]; then
		uuid="da594c53-9beb-f85c-85c5-cedf76546f7a"
	else
		uuid=$(cat /proc/sys/kernel/random/uuid)
	fi

	echo "mount_point=${part_name}" > "${file}"
	echo "fs_type=${fs_type}" >> "${file}"
	echo "partition_size=${partition_align_size}" >> "${file}"
	echo "partition_name=${part_name}" >> "${file}"
	echo "uuid=${uuid}" >> "${file}"
	echo "ext_mkuserimg=${build_image_tools}/mkuserimg_mke2fs" >> "${file}"

	echo "verity_block_device=/dev/mmcblk0p${part_id}" >> "${file}"
	echo "skip_fsck=true" >> "${file}"
}

if [ -z "${HR_BD_IMG_TOOLS_PATH}" ]; then
	echo "[ERROR]: build_image tools path is not exist"
	exit 1
fi

# export tools
build_image_tools="${HR_BD_IMG_TOOLS_PATH}/bin"

part_name="$1"
indir=${2}
avb_out_dir=${HR_TARGET_DEPLOY_DIR}/vbmeta/
mkdir -p "${avb_out_dir}"

config_file="${avb_out_dir}/${part_name}_image_info.txt"

create_config_file "$config_file"

# get $indir $outimage $partition_align_size in function create_config_file
echo "python3 ${HR_BD_IMG_TOOLS_PATH}/build_image.py $indir $config_file $outimage $partition_align_size"
if [ -d "${HR_TARGET_DEPLOY_DIR}"/tmp ];then
	rm -rf "${HR_TARGET_DEPLOY_DIR}"/tmp
fi

cp -a "${indir}" "${HR_TARGET_DEPLOY_DIR}"/tmp
find "${HR_TARGET_DEPLOY_DIR}"/tmp -type d -name "include" -print0 | xargs -0 rm -rf
find "${HR_TARGET_DEPLOY_DIR}"/tmp -type f -name "*.a" -print0 | xargs -0 rm -f
if [ "${HR_TARGET_MODE}" = "release" ];then
	strip_elf "${HR_TARGET_DEPLOY_DIR}"/tmp
fi
python3 "${HR_BD_IMG_TOOLS_PATH}"/build_image.py "${HR_TARGET_DEPLOY_DIR}"/tmp "${config_file}" "${outimage}" "${partition_align_size}"
rm -rf "${HR_TARGET_DEPLOY_DIR}"/tmp
