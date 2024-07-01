#!/bin/bash

# Define bd.sh command shortcuts
function b() {
	(set -x; bd.sh "$@")
}

function ball() {
	(set -x; bd.sh all)
}

function bm() {
	(set -x; bd.sh miniboot "$@")
}

function bu() {
	(set -x; bd.sh uboot "$@")
}

function bumc() {
	(set -x; bd.sh uboot menuconfig)
}

function bf() {
	(set -x; bd.sh factory "$@")
}

function bb() {
	(set -x; bd.sh boot "$@")
}

function bbmc() {
	(set -x; bd.sh boot menuconfig)
}

function bs() {
	(set -x; bd.sh system "$@")
}

function bh() {
	(set -x; bd.sh hbre "$@")
}

function bhm() {
	(set -x; bd.sh hbre module "$@")
}

function ba() {
	(set -x; bd.sh app "$@")
}

function bam() {
	(set -x; bd.sh app module "$@")
}

function bp() {
	(set -x; bd.sh pack "$@")
}

function croot() {
	\cd "${HR_TOP_DIR}" || return
}

function cr() {
	\cd "${HR_TOP_DIR}" || return
}

function cout() {
	\cd "${HR_TOP_DIR}"/out || return
}

function co() {
	\cd "${HR_TOP_DIR}"/out || return
}

function cuboot() {
	\cd "${HR_TOP_DIR}"/uboot || return
}

function cub() {
	\cd "${HR_TOP_DIR}"/uboot || return
}

function cboot() {
	\cd "${HR_TOP_DIR}"/kernel || return
}

function cb() {
	\cd "${HR_TOP_DIR}"/kernel || return
}

function cdev() {
	\cd "${HR_TOP_DIR}"/device || return
}

function cbuild() {
	\cd "${HR_TOP_DIR}"/build || return
}

function capp() {
	\cd "${HR_TOP_DIR}"/app || return
}

function chbre() {
	\cd "${HR_TOP_DIR}"/hbre || return
}

function go()
{
	if [[ -z "$1" ]]; then
		echo "Usage: go <regex>"
		return
	fi
	T=${HR_TOP_DIR}
	if [ -z "${HR_TOP_DIR}" ]; then
		echo "[ERROR]: HR_TOP_DIR is not set. Please run 'source qiuckcmd.sh'."
		return
	fi

	if [ ! -d "${HR_TOP_DIR}" ]; then
		echo "[ERROR]: Directory ${HR_TOP_DIR} does not exist. Please run 'source qiuckcmd.sh'."
		return
	fi

	if [[ ! -f $T/.filelist ]]; then
		echo -n "Creating file list index..."
		(\cd "$T" || return; find . -wholename ./out -prune -o -wholename ./.repo -prune -o -wholename ./sdks -prune -o -type d > .filelist)
		echo " Done"
		echo ""
	fi
	local lines
	mapfile -t lines < <(\grep "/[-a-z0-9A-Z_]*$1[-a-z0-9A-Z_\.]*$" "$T/.filelist" | sort | uniq)
	if [[ ${#lines[@]} = 0 ]]; then
		echo "[ERROR]: $1 not found."
		return
	fi
	local pathname
	local choice
	if [[ ${#lines[@]} -gt 1 ]]; then
		while [[ -z "$pathname" ]]; do
			local index=1
			local line
			for line in "${lines[@]}"; do
				printf "%6s -- %s -- %s\n" "[$index]" "$line" "[$index]"
				index=$((index + 1))
			done
			echo
			echo -n "Select one: "
			unset choice
			read -r choice
			if [[ $choice -gt ${#lines[@]} || $choice -lt 1 ]]; then
				echo "Invalid choice"
				continue
			fi
			pathname=${lines[$((choice-1))]}
		done
	else
		pathname=${lines[0]}
	fi
	\cd "$T"/"$pathname" || return
}

function shortcuts_help {
	echo -e "Available commands for bd.sh:"
	echo -e "\t${avail_cmds}\n"

	echo -e "Shortcut commands for build:"
	echo -e "\tb      : bd.sh             - default build all"
	echo -e "\tball   : bd.sh all         - build all"
	echo -e "\tbm     : bd.sh miniboot    - only build miniboot"
	echo -e "\tbu     : bd.sh uboot       - only build uboot"
	echo -e "\tbf     : bd.sh factory     - build uart_usb image"
	echo -e "\tbb     : bd.sh boot        - only build kernel"
	echo -e "\tbs     : bd.sh system      - only build rootfs"
	echo -e "\tbh     : bd.sh hbre        - build hbre"
	echo -e "\tbhm    : bd.sh hbre module - build hbre module, user interactive mode"
	echo -e "\tba     : bd.sh app         - build app"
	echo -e "\tbam    : bd.sh app module  - build app module, user interactive mode"
	echo -e "\tbp     : bd.sh pack        - pack all image into emmc_disk.img"
	echo -e ""
	echo -e "Shortcut commands for changing directory:"
	echo -e "\tcroot  - go to root directory"
	echo -e "\tcr     - go to root directory"
	echo -e "\tcout   - go to out directory"
	echo -e "\tco     - go to out directory"
	echo -e "\tcuboot - go to uboot directory"
	echo -e "\tcub    - go to uboot directory"
	echo -e "\tcboot  - go to kernel directory"
	echo -e "\tcb     - go to kernel directory"
	echo -e "\tcdev   - go to device directory"
	echo -e "\tcbuild - go to device directory"
	echo -e "\tcapp   - go to app directory"
	echo -e "\tchbre  - go to hbre directory"
	echo -e "\tgo <regex> -- go to directory matching the specified <regex>"
	echo -e ""
	echo -e "Shortcut commands for configuring U-Boot and the kernel defconfig"
	echo -e "\tbumc   : bd.sh uboot menuconfig       - Edit and save uboot menuconfig"
	echo -e "\tbbmc   : bd.sh boot menuconfig        - Edit and save kernel menuconfig"
	echo -e ""
	echo -e "Usage example for build kernel and hbre module"
	echo -e "\tbb spi      : bd.sh boot spi          - Compile driver under kernel"
	echo -e "\tbh liblog   : bd.sh hbre liblog       - Compile modules under hbre"
}

SCRIPT_DIR="$( cd "$( dirname "$(readlink -f "${BASH_SOURCE[0]}")" )" && pwd )"
HR_TOP_DIR=$(realpath "${SCRIPT_DIR}"/../)
export HR_TOP_DIR
export PATH="${HR_TOP_DIR}:${PATH}"

# Your existing quickcmd.sh content goes here
if [[ "$PS1" != *"<xbuild>"* ]]; then
    export PS1="<xbuild>${PS1}"
fi

# configure compilation parameter completion
avail_cmds="help all clean distclean lunch miniboot uboot factory boot system hbre app pack"
complete -W "${avail_cmds}" bd.sh
complete -W "${avail_cmds}" b

echo -e "=================================================================================="
echo -e "   \\  //   Welcome to the D-Robotics xbuild system!"
echo -e "    \\//    Working directory: ${HR_TOP_DIR}"
echo -e "    //\\                                             "
echo -e "   //  \\                                            "
echo -e "=================================================================================="
shortcuts_help
echo -e "=================================================================================="

