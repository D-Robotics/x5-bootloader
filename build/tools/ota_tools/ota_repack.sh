#!/bin/bash


SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

function print_help()
{
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  -unpack(-x), --unpack <OTA_FILE>     Unpack an OTA package file."
    echo "  -repack(-c), --repack                Repack the OTA package."
    echo "  -h, --help                       Display this help message and exit."
}

function unpack()
{
    local ota_file="$1"

    if [ ! -f "$ota_file" ]; then
        echo "Error: OTA package file '$ota_file' does not exist."
        exit 1
    fi

    ${SCRIPT_DIR}/mk_otapackage.py unpack \
        --ota_pkg $ota_file \
        --out_dir "${SCRIPT_DIR}/out/ota_unpack"
}

function repack()
{
    ${SCRIPT_DIR}/mk_otapackage.py repack \
        --partition_file "${SCRIPT_DIR}/j6e-gpt.json" \
        --ota_process ${SCRIPT_DIR}/out/ota_unpack/ota_process \
        --image_dir "${SCRIPT_DIR}/out/ota_unpack" \
        --prepare_dir "${SCRIPT_DIR}/out/ota_deploy" \
        --sign_key ${SCRIPT_DIR}/private_key.pem \
        --out_dir "${SCRIPT_DIR}/out/ota_repack"
}

if [ "$#" -lt 1 ]; then
    print_help
    exit 1
fi

while [ "$#" -gt 0 ]; do
    case "$1" in
        -x|unpack|-unpack|--unpack)
            if [ "$#" -lt 2 ]; then
                echo "Error: -unpack requires an OTA package file."
                print_help
                exit 1
            fi

            ota_file="$2"
            shift 2
            unpack "$ota_file"
            ;;
        -c|repack|-repack|--repack)
            if [ "$#" -gt 1 ]; then
                echo "Error: Invalid option"
                print_help
                exit 1
            fi
            repack
            shift
            ;;
        -h|-help|--help)
            print_help
            exit 0
            ;;
        *)
            echo "Error: Invalid option: $1"
            print_help
            exit 1
            ;;
    esac
done
