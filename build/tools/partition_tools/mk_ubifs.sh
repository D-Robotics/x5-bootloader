#!/bin/bash -e
# this file build ubifs system.img

# When unexpected situations occur during script execution, exit immediately to avoid errors being ignored and incorrect final results
set -e

################### setting utils_funcs ###################
source "${HR_TOP_DIR}/build/utils_funcs.sh"

function gne_ubi_boot_img_config()
{
	local cfg_file=$1
	cat > "${cfg_file}" << EOF
[ubootenv]
mode=ubi
image=ubootenv.img
vol_id=0
vol_size=256KiB
vol_type=dynamic
vol_alignment=1
vol_name=ubootenv

[vbmeta]
mode=ubi
image=vbmeta.img
vol_id=1
vol_size=16KiB
vol_type=static
vol_alignment=1
vol_name=vbmeta

[boot]
mode=ubi
image=boot.img
vol_id=2
vol_size=10MiB
vol_type=static
vol_alignment=1
vol_name=boot

EOF
}

function gne_ubi_fs_img_config()
{
	local cfg_file=$1
	local vol_id=$2
	cat > "${cfg_file}" << EOF
[${part_name}]
mode=ubi
image=ubi/${part_name}.ubifs
vol_id=${vol_id}
vol_type=dynamic
vol_alignment=1
vol_name=${part_name}
vol_flags=autoresize

EOF
}

function gne_ubi_system_config()
{
	local cfg_file=$1
	local img_size=$2
	cat > "${cfg_file}" << EOF
[system]
mode=ubi
image=ubi/system.ubifs
vol_id=0
vol_type=dynamic
vol_alignment=1
vol_name=system
vol_flags=autoresize

EOF
}

# Reference link [http://www.linux-mtd.infradead.org/doc/ubi.html]
# ubi_overhead=(B + 4) * SP + O * (P - B - 4)
function cal_leb() {
	local avail_size=$1
	local peb_size=$2
	local page_size=$3
	#calculate the leb available for the nand size passed in
	avail_peb=$(($avail_size / $peb_size))
	if [ "${medium}" = "nand" ];then
		peb_reserved=$(($avail_peb / 50 + 1))
		ubi_overhead=$(((2 * $page_size * ($avail_peb - $peb_reserved - 4) + $peb_size * ($peb_reserved + 4)) / $peb_size + 1))
	else
		peb_reserved=0
		ubi_overhead=$(((128 * ($avail_peb - $peb_reserved - 4) + $peb_size * ($peb_reserved + 4)) / $peb_size + 1))
	fi

	avail_leb=$(($avail_peb - $ubi_overhead))
	echo $avail_leb
}

function gen_nand() {
	local part_name="$1"
	local page_size=${NAND_PAGE_SIZE}
	local nand_size=${NAND_SIZE}
	local out_dir=${HR_TARGET_PRODUCT_DIR}/
	local indir="$2"
	local peb_size=$((${page_size} * 64))
	local leb_size=$((${page_size} * 62))
	local fs_type=$(get_part_attr "${part_name}" fs_type)

	mkdir -p "${out_dir}"/ubi

	if [ "${fs_type}" = "ubifs" ];then
		if [ "${part_name}" = "userdata" ]; then
			local img_start=$(get_part_attr "${part_name}" start)
			local img_size=$((${nand_size} - ${img_start}))
			rm -f "${out_dir}/${part_name}.img"
			dd if=/dev/zero ibs=1K count=$((${img_size} /1024 )) | tr "\000" "\377" > "${out_dir}/${part_name}.img"
		else
			local img_size=$(get_part_attr "${part_name}" size)
			dd if=/dev/zero ibs=1M count=$((${img_size} / 1024 / 1024 - 1)) | tr "\000" "\377" > "${out_dir}/${part_name}.img"
		fi
		local imgleb=$(cal_leb "${img_size}" "${peb_size}" "${page_size}")
		# Actual start of nand image build
		echo "${part_name}: ${img_size}"
		echo "LEB used for ${part_name}.ubi: ${imgleb}"
		# prepare each volume in ubi, including creating ubifs images
		mkfs.ubifs -r "${indir}/" -m "${page_size}" -e "${leb_size}" -c "${imgleb}" -o "${out_dir}/ubi/${part_name}".ubifs
	fi

	if [ "${part_name}" = "system" ]; then
		gne_ubi_system_config "${out_dir}/ubi/${part_name}".cfg "${img_size}"
	elif [ "${part_name}" = "boot" ]; then
		dd if=/dev/zero ibs=256K count=1 | tr "\000" "\377" > "${out_dir}/ubootenv.img"
		gne_ubi_boot_img_config "${out_dir}/ubi/${part_name}".cfg
	else
		system_id=$(get_part_attr system part_num)
		cur_img_id=$(get_part_attr "${part_name}" part_num)
		gne_ubi_fs_img_config "${out_dir}/ubi/${part_name}".cfg $((${cur_img_id} - ${system_id}))
	fi
	pushd "${out_dir}/" > /dev/zero || exit 1
	ubinize -o "${out_dir}/ubi/${part_name}".ubi -m "${page_size}" -p "${peb_size}" -v "${out_dir}/ubi/${part_name}".cfg
	popd > /dev/zero || exit 1

	dd if="${out_dir}/ubi/${part_name}.ubi" of="${out_dir}/${part_name}.img" conv=notrunc,sync status=none

	echo "${part_name} ubi image done"
}

function gen_nor() {
	local part_name="$1"
	local nor_size=${NOR_SIZE}
	local out_dir=${HR_TARGET_PRODUCT_DIR}/
	local indir="$2"
	local peb_size=65536
	local leb_size=65408
	local fs_type=$(get_part_attr "${part_name}" fs_type)

	mkdir -p "${out_dir}"/ubi

	if [ "${fs_type}" = "ubifs" ];then
		if [ "${part_name}" = "userdata" ]; then
			local img_start=$(get_part_attr "${part_name}" start)
			local img_size=$((${nor_size} - ${img_start}))
			rm -f "${out_dir}/${part_name}.img"
			dd if=/dev/zero ibs=1K count=$((${img_size} /1024 )) | tr "\000" "\377" > "${out_dir}/${part_name}.img"
		else
			local img_size=$(get_part_attr "${part_name}" size)
			dd if=/dev/zero ibs=1M count=$((${img_size} / 1024 / 1024 - 1)) | tr "\000" "\377" > "${out_dir}/${part_name}.img"
		fi
		local imgleb=$(cal_leb "${img_size}" "${peb_size}" "")
		# Actual start of nor image build
		echo "${part_name}: ${img_size}"
		echo "LEB used for ${part_name}.ubi: ${imgleb}"
		# prepare each volume in ubi, including creating ubifs images
		mkfs.ubifs -r "${indir}/" -m 1 -e "${leb_size}" -c "${imgleb}" -o "${out_dir}/ubi/${part_name}".ubifs
	fi

	if [ "${part_name}" = "system" ]; then
		gne_ubi_system_config "${out_dir}/ubi/${part_name}".cfg "${img_size}"
	elif [ "${part_name}" = "boot" ]; then
		dd if=/dev/zero ibs=256K count=1 | tr "\000" "\377" > "${out_dir}/ubootenv.img"
		gne_ubi_boot_img_config "${out_dir}/ubi/${part_name}".cfg
	else
		system_id=$(get_part_attr system part_num)
		cur_img_id=$(get_part_attr "${part_name}" part_num)
		gne_ubi_fs_img_config "${out_dir}/ubi/${part_name}".cfg $((${cur_img_id} - ${system_id}))
	fi
	pushd "${out_dir}/" > /dev/zero || exit 1
	ubinize -o "${out_dir}/ubi/${part_name}".ubi -m 1 -p "${peb_size}" -s 1 -v "${out_dir}/ubi/${part_name}".cfg
	popd > /dev/zero || exit 1

	dd if="${out_dir}/ubi/${part_name}.ubi" of="${out_dir}/${part_name}.img" conv=notrunc,sync status=none

	echo "${part_name} ubi image done"
}

part_name=$1
indir=$2
medium=$(get_part_attr "${part_name}" medium)

if [ -d "${HR_TARGET_DEPLOY_DIR}"/tmp ];then
	rm -rf "${HR_TARGET_DEPLOY_DIR}"/tmp
fi

if [ -d "${indir}" ];then
	cp -a "${indir}" "${HR_TARGET_DEPLOY_DIR}"/tmp
	find "${HR_TARGET_DEPLOY_DIR}"/tmp -type d -name "include" -print0 | xargs -0 rm -rf
	find "${HR_TARGET_DEPLOY_DIR}"/tmp -type f -name "*.a" -print0 | xargs -0 rm -f
fi

if [ "${HR_TARGET_MODE}" = "release" ];then
	strip_elf "${HR_TARGET_DEPLOY_DIR}"/tmp
fi

if [ "${medium}" = "nand" ];then
	gen_nand "${part_name}" "${HR_TARGET_DEPLOY_DIR}"/tmp
elif [ "${medium}" = "nor" ];then
	gen_nor "${part_name}" "${HR_TARGET_DEPLOY_DIR}"/tmp
fi
rm -rf "${HR_TARGET_DEPLOY_DIR}"/tmp
