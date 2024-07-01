#!/bin/bash

# When unexpected situations occur during script execution, exit immediately to avoid errors being ignored and incorrect final results
set -e

PWD=`pwd`

if [ -z "${CROSS_COMPILE}" ]; then
    TOOLCHAIN_PATH=/opt/arm-gnu-toolchain-11.3.rel1-x86_64-aarch64-none-linux-gnu
    export CROSS_COMPILE=${TOOLCHAIN_PATH}/bin/aarch64-none-linux-gnu-
fi

export arch=arm64
export TA_DEV_KIT_DIR=$PWD/export-ta_${arch}

echo "CROSS_COMPILE is set to ${CROSS_COMPILE}"

export CROSS_COMPILE_HOST=${CROSS_COMPILE}
export CROSS_COMPILE_TA=${CROSS_COMPILE}
export CROSS_COMPILE_user_ta=${CROSS_COMPILE}

function all
{
	echo "compile ta"

	make all
}

function clean
{
	echo "clean ta"
	make clean
}

cmd=$1

if [ "$cmd" = "clean" ]
then
    clean
else
    # start to build
    all
fi
