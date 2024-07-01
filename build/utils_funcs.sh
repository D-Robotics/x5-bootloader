#!/bin/bash

function lunch_usage()
{
	echo "Usage: ./xbuild.sh lunch [0-9] | [name of board config]"
	echo "    ./xbuild.sh lunch"
	echo "    ./xbuild.sh lunch 1"
	echo "    ./xbuild.sh lunch board_config.mk"
	echo
	exit 1
}

function build_env_setup(){
	# Check if the cross-compilation toolchain is installed
	if [ -d "${TOOLCHAIN_PATH}" ]; then
		if [ -x "${CROSS_COMPILE}gcc" ]; then
			echo "[INFO] Cross-compiler found: ${CROSS_COMPILE}gcc"
		else
			echo "[ERROR]: Cross-compiler not found. Please make sure the toolchain is properly installed and the CROSS_COMPILE variable is set correctly."
			echo "         Current value of CROSS_COMPILE: ${CROSS_COMPILE}"
			exit 1
		fi
	else
		echo "[ERROR]: Cross-compiler toolchain directory not found. Please set the correct TOOLCHAIN_PATH."
		echo "         Current value of TOOLCHAIN_PATH: ${TOOLCHAIN_PATH}"
		exit 1
	fi
	echo "declare -x ARCH=${ARCH}"
	echo "declare -x CROSS_COMPILE=${CROSS_COMPILE}"
	export | grep " HR_"
	# configure ccache
	if [ "${HR_CCACHE_SUPPORT}" = "y" ] && [ "${HR_CCACHE_COMMAND}" = "ccache" ]; then
		echo "[INFO]: Enable compiled ccache function."
		mkdir -p "${HR_CCACHE_DIR}"
		${CCACHE_COMMAND} -M 10G
	fi
}

function lunch_board_combo()
{
	echo -e "\nYou're building on $(uname -v)"

	LUNCH_MENU_CHOICES=( $(cd ${HR_TOP_DIR}/device; find -L -iname board*.mk | cut -c 3- | sort) )
	BOARD_CONFIG=${HR_TOP_DIR}/device/.board_config.mk

	if [ $# == 0 ]; then
		echo "Lunch menu... pick a combo:"
		local i=0
		local choice
		for choice in "${LUNCH_MENU_CHOICES[@]}"
		do
			echo "      $i. $choice"
			i=$((i+1))
		done

		local answer
		echo -n "Which would you like? [0] : "
		read -r answer

		if [[ "$answer" =~ ^[0-9]+$ ]]; then
			echo "You are selected board config: ${LUNCH_MENU_CHOICES[answer]}"
			ln -rfs  "${HR_TOP_DIR}"/device/"${LUNCH_MENU_CHOICES[answer]}" "$BOARD_CONFIG"
		else
			echo -e "[Error] The input is not a integer.Please input a valid integer.\n"
			exit 1
		fi
	elif [ $# == 1 ]; then
		if [[ "$1" =~ ^[0-9]+$ ]]; then
			if [ "$1" -ge ${#LUNCH_MENU_CHOICES[@]} ]; then
				echo -e "[ERROR]: Option '$1' out of range, please retry\n"
				exit 1
			fi
			echo "You are selected board config: ${LUNCH_MENU_CHOICES[$1]}"
			ln -rfs  "${HR_TOP_DIR}"/device/"${LUNCH_MENU_CHOICES[$1]}" "$BOARD_CONFIG"
		elif [[ "$1" = "help" ]]; then
			lunch_usage
		else
			if [[ ! "${LUNCH_MENU_CHOICES[*]}" =~ ${1} ]]; then
				echo "[ERROR]: Board config '$1' not found."
				echo "You should select config from the following configurations:"
				i=0
				for choice in "${LUNCH_MENU_CHOICES[@]}"
				do
					echo "      $i. $choice"
					i=$((i+1))
				done
				lunch_usage
			fi
			i=0
			for choice in "${LUNCH_MENU_CHOICES[@]}"
			do
				if [[ "${choice}" =~ ${1} ]]; then
					echo "You are selected board config: ${choice}"
					ln -rfs  "${HR_TOP_DIR}"/device/"${choice}" "$BOARD_CONFIG"
					break
				fi
				i=$((i+1))
			done
		fi
	else
		echo "[ERROR]: Options not supported"
		echo "Options should be a valid integer or name of board config."
		lunch_usage
	fi
	echo
}

function check_board_config()
{
	BOARD_CONFIG=${HR_TOP_DIR}/device/.board_config.mk
	if [ $# == 1 ] && { [ "$1" = "clean" ] || [ "$1" = "distclean" ]; } && [ ! -L "${BOARD_CONFIG}" ]; then
		exit 1
	fi

	[ ! -L "${BOARD_CONFIG}" ] && {
		lunch_board_combo "${@}"
	}

	[ -z "${HR_IS_BOARD_CONFIG_EXPORT}" ] && source "${BOARD_CONFIG}"

	return 0
}

function cpfiles()
{
	if [ $# -ne 2 ];then
		echo "Usage: cpfiles \"sourcefiles\" \"destdir\""
		exit 1
	fi

	mkdir -p "$2" || {
		echo "mkdir -p $2 failed"
		exit 1
	}

	for f in $1
	do
		if [ -a "$f" ];then
			cp -af "$f" "$2" || {
				echo "[ERROR]: cp -af $f $2 failed"
				exit 1
			}
		fi
	done
	echo "[INFO]: cpfiles $1 $2"
}

function move_file()
{
	target_root=$1
	manifest=$2
	cat ${manifest} | while read file;
	do
		src_file=${file%=>*}
		dst_file=${file#*=>}
		echo "mv ${src_file} => ${dst_file}"
		mv ${target_root}/${src_file} ${target_root}/${dst_file}
	done
}

function inList()
{
	if [ $# -ne 2 ];then
		echo "Usage: inList element list"
		exit 1
	fi

	local arr=$2
	local result=1
	local elem

	for elem in ${arr[*]}
	do
		if [ "$elem" = "$1" ];then
			result=0
			break
	   fi
	done

	return $result
}

function runcmd()
{
	if [ $# -ne 1 ];then
		echo "Usage: runcmd command_string"
		exit 1
	fi

	echo "$1"
	$1 || {
		echo "failed"
		exit 1
	}
}

function get_part_name_list() {
	if ! result=$("${HR_PARTITION_TOOL_PATH}"/GPTParse.py -l); then
		echo "[ERROR]: Unable to execute GPTParse.py -l. Exiting."
		exit 1
	fi

	echo "${result}"
}

function get_miniboot_list() {
	if ! result=$("${HR_PARTITION_TOOL_PATH}"/GPTParse.py -g); then
		echo "[ERROR]: Unable to execute GPTParse.py -g. Exiting."
		exit 1
	fi

	echo "${result}"
}

function get_mtd_part_list() {
	if ! mtd_part_list=$("${HR_PARTITION_TOOL_PATH}"/GPTParse.py -m); then
		echo "[ERROR]: Unable to execute GPTParse.py -m. Exiting."
		exit 1
	fi

	echo "${mtd_part_list}"
}

function get_part_attr()
{
	local part=$1
	local attr=$2
	if ! result=$("${HR_PARTITION_TOOL_PATH}"/GPTParse.py -s "${part}:${attr}"); then
		echo "[ERROR]: Unable to execute GPTParse.py -s ${part}:${attr}. Exiting."
		exit 1
	fi

	echo "${result}"
}

function strip_elf() {
	local ori_dir=$1
	for f in $(find ${ori_dir}/ -type f -print | grep -v ".ko"); do
		fm=$(file $f)
		slim=${fm##*, }
		if [ "${slim}" = "not stripped" ]; then
			${CROSS_COMPILE}strip $f
		fi
	done
}
