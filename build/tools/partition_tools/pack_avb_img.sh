#!/bin/bash
# This file is to pack boot.img recovery.img

# When unexpected situations occur during script execution, exit immediately to avoid errors being ignored and incorrect final results
set -e

################### setting utils_funcs ###################
source "${HR_TOP_DIR}/build/utils_funcs.sh"

function pack_avb_boot()
{
	local part_name=$1
	local avb_out_dir=${HR_TARGET_DEPLOY_DIR}/vbmeta
	local boot_img=${HR_TARGET_PRODUCT_DIR}/${part_name}.img
	boot_size=$(get_part_attr "${part_name}" size)

	mkdir -p "${avb_out_dir}"
	echo "${HR_AVB_TOOLS_PATH}/avbtool add_hashtree_footer to ${part_name}.img"
	python3 ${HR_AVB_TOOLS_PATH}/avbtool add_hash_footer \
		--hash_algorithm sha256 \
		--partition_name "${part_name}" \
		--partition_size "${boot_size}" \
		--image "${boot_img}" \
		--output_vbmeta_image "${avb_out_dir}/vbmeta_${part_name}.img" \
		--key ${HR_AVB_TOOLS_PATH}/keys/shared.priv.pem \
		--algorithm SHA256_RSA2048 \
		--kernel_cmdline "bootverify"
}

function pack_avb_fs()
{
	local part_name=$1
	local cmdline=$2
	local avb_out_dir=${HR_TARGET_DEPLOY_DIR}/vbmeta
	local part_img=${HR_TARGET_PRODUCT_DIR}/${part_name}.img
	part_size=$(get_part_attr "${part_name}" size)

	mkdir -p "${avb_out_dir}"
	echo "avbtool add_hashtree_footer to ${part_name}.img part_size="${part_size}" cmdline=${cmdline}"
	python3 ${HR_AVB_TOOLS_PATH}/avbtool add_hashtree_footer \
		--partition_name "${part_name}" \
		--partition_size "${part_size}" \
		--image "${part_img}" \
		--do_not_generate_fec \
		--do_not_append_vbmeta_image \
		--no_hashtree \
		--internal_release_string \"\"  \
		--kernel_cmdline "${cmdline}" \
		--output_vbmeta_image "$avb_out_dir/vbmeta_${part_name}.img" || {
			echo "make ${part_name} vbmeta add footer failed!"
			exit 1
		}
}

function pack_avb_vbmeta()
{
	local part_name=$1
	local avb_out_dir=${HR_TARGET_DEPLOY_DIR}/vbmeta
	local vbmeta_img=${HR_TARGET_PRODUCT_DIR}/${part_name}.img
	local include_boot="--include_descriptors_from_image ${avb_out_dir}/vbmeta_boot.img"
	local include_system="--include_descriptors_from_image ${avb_out_dir}/vbmeta_system.img"

	python3 ${HR_AVB_TOOLS_PATH}/avbtool make_vbmeta_image \
		--output ${vbmeta_img} \
		--algorithm SHA256_RSA2048 \
		--key ${HR_AVB_TOOLS_PATH}/keys/shared.priv.pem \
		${include_boot} \
		${include_system} || {
			echo "Failed! ******************"
			exit 1
		}
}

function parse_value()
{
	local hash_txt=$1
	local label=$2
	cat ${hash_txt} |grep "${label}" |awk -F: '{print $2}'| sed 's/^[ \t]*//;s/[ \t]*$//'
}

function gen_dm_verity_cmdline()
{
	local part_name=$1
	local part_img=${HR_TARGET_PRODUCT_DIR}/${part_name}.img
	local hash_txt=${HR_TARGET_DEPLOY_DIR}/vbmeta/hash_${part_name}.txt
	local part_size=$(get_part_attr "${part_name}" "size")
	#local hash_size=$((8 * 1024 * 1024))
	local hash_size=$(( ${part_size} / 1024 / 1024 / 64 * 1024 * 1024)) 
	resize2fs -M "${part_img}"
	local image_size=$(stat -c "%s" "${part_img}")
	local max_img_size=$((${part_size} - ${hash_size} - 4096))
	local max_img_blocks=$((${max_img_size} / 512))
	if [ ${image_size} -gt ${max_img_size} ];then
		echo "[avb error]: image size [${image_size}] + hash size [${hash_size}] > partition size [${part_size}]"
		exit 1
	fi
	local hash_start_bytes=$((${part_size} - ${hash_size}))
	local hash_start_block=$((${hash_start_bytes} / 4096 + 1))
	local data_blocks=$((${max_img_size} / 4096 + 1))
	resize2fs "${part_img}" "${max_img_blocks}s"

	veritysetup -v --debug format "${part_img}" "${part_img}" --data-blocks=${data_blocks} --hash-offset=${hash_start_bytes} > "${hash_txt}"
	local roothash=$(parse_value "${hash_txt}" "Root")
	local hashtype=$(parse_value "${hash_txt}" "Hash type")
	local data_blks=$(parse_value "${hash_txt}" "Data blocks")
	local data_blk_size=$(parse_value "${hash_txt}" "Data block size")
	local hash_blk_size=$(parse_value "${hash_txt}" "Hash block size")
	local algorithm=$(parse_value "${hash_txt}" "Hash algorithm")
	local salt=$(parse_value "${hash_txt}" "Salt")
	local data_len=$((${image_size} / 512))

	cmdline="dm-verity,,,ro,0 ${max_img_blocks} verity ${hashtype} \$(SYSTEM_PART) \$(SYSTEM_PART) ${data_blk_size} ${hash_blk_size} ${data_blks} ${hash_start_block} ${algorithm} ${roothash} ${salt}"
	cmdline=$(echo dm-mod.create=\"${cmdline} 2 restart_on_corruption ignore_zero_blocks\")
	echo "${cmdline}" >> "${hash_txt}"
	echo "${cmdline}"
}

function gen_crypt_cmdline()
{
	local part_name=$1
	local part_img=${HR_TARGET_PRODUCT_DIR}/${part_name}.img
	local encrypt_img=${HR_TARGET_PRODUCT_DIR}/${part_name}encrypt.img
	local enc_key_file=${HR_TARGET_DEPLOY_DIR}/vbmeta/fde-encrypt-128.key
	local ori_key=${HR_BOARD_CONF_DIR}/key_files/fde-origin-128.key
	local user_root_key=${HR_BOARD_CONF_DIR}/bl2_cfg/user_root.key
	local part_size=$(get_part_attr "${part_name}" size)
	local image_size=$(stat -c "%s" "${part_img}")
	local en_img_blocks=$((${part_size} / 512 - 1024))
	local ori_key_str=$(xxd -p "${ori_key}" | tr -d '\n')

	openssl enc -aes-128-ecb -in "${ori_key}" -out "${enc_key_file}" -K $(xxd -p "${user_root_key}" | tr -d '\n') -nopad

	local enc_key_str=$(xxd -p "${enc_key_file}" | tr -d '\n')

	if [ ${image_size} -gt $((${en_img_blocks} * 512)) ];then
		echo "[avb error]: image size [${image_size}] + 512k > partition size [${part_size}]"
		exit 1
	fi

	test -e "${encrypt_img}" && rm "${encrypt_img}" -f
	dd if=/dev/zero of="${encrypt_img}" bs=512 count="${en_img_blocks}"
	resize2fs "${part_img}" "${en_img_blocks}s"
	loopdevice=$(losetup -f)
	sudo losetup "${loopdevice}" "${encrypt_img}"
	mappername=encfs-$(shuf -i 1-10000000000000000000 -n 1)
	if [ -b /dev/mapper/"${mappername}" ];then
		sudo dmsetup remove "${mappername}"
	fi
	sudo dmsetup create "${mappername}" --table "0 ${en_img_blocks} crypt aes-cbc-essiv:sha256 ${ori_key_str} 0 ${loopdevice} 0"
	sudo dd if="${part_img}" of=/dev/mapper/"${mappername}" bs=512
	sync
	sudo dmsetup remove "${mappername}"
	sudo losetup -d "${loopdevice}"

	mv "${encrypt_img}" "${part_img}"

	cmdline="dm-crypt,,0,ro,0 ${en_img_blocks} crypt aes-cbc-essiv:sha256 dr-fde:${enc_key_str} 0 \$(SYSTEM_PART) 0"
	cmdline=$(echo dm-mod.create=\"${cmdline} 1 allow_discards \")
	echo ${cmdline}
}

if [ -z "${HR_AVB_TOOLS_PATH}" ]; then
	echo "${HR_AVB_TOOLS_PATH}/avbtool path is not exist"
	exit 1
fi

cd "${SRC_AVBTOOLS_DIR}/"

part_type=$1
part_name=$2
cmdline="-"

if [ "${part_type}" = "boot" ]; then
	pack_avb_boot "${part_name}"
elif [ "${part_type}" = "fs" ]; then
	if [ "${part_name}" = "system" ]; then
		if [ "${HR_SYSTEM_VERIFY}" = "dm-verity" ];then
			gen_dm_verity_cmdline "${part_name}"
		elif [ "${HR_SYSTEM_VERIFY}" = "crypt" ];then
			gen_crypt_cmdline "${part_name}"
		fi
	fi
	pack_avb_fs "${part_name}" "${cmdline}"
elif [ "${part_type}" = "vbmeta" ]; then
	pack_avb_vbmeta "${part_name}"
else
	echo "${part_type} error"
fi
