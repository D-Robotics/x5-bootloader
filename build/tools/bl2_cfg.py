#!/usr/bin/env python3
#
# Copyright 2023, ming.yu@horizon.cc
#

import json
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.backends import default_backend
import hashlib
import struct
import sys
import os

bl2_offset = struct.calcsize("QIIIIIIIII")
bl2_feature_size = struct.calcsize("QII160s")
ddr_input_size = struct.calcsize("6I")
efuse_data_size = struct.calcsize("II")
efuse_cfg_size = struct.calcsize("QIIIIIIIIIII16sBBBBBBBBIi")
socid_size = 16 * 100


def parse_socid(json_file, socid_file):
    with open(json_file, 'r') as file:
        json_data = json.load(file)

    socid_list = json_data['bl2_cfg']['socid']

    # write to a temp file
    with open(socid_file, 'wb') as hex_file:
        for socid in socid_list:
            id_str = socid[2:]
            id_str = id_str.rjust(32, '0')
            id_hex = bytes.fromhex(id_str)[::-1]
            hex_file.write(id_hex)

    # checksum
    checksum_socid = 0
    for socid in socid_list:
        for i in range(2, len(socid), 2):  # 从第一个字符开始，每两个字符为一个字节
            byte_value = int(socid[i:i+2], 16)  # 解析两个字符为一个字节的十六进制数值
            checksum_socid += byte_value

    # print("Checksum:", checksum_socid)
    return checksum_socid


def calculate_public_key_hash(key_file):
    with open(key_file, "rb") as key_file:
        private_key = serialization.load_pem_private_key(
            key_file.read(),
            password=None,
            backend=default_backend()
        )

    public_key = private_key.public_key()
    public_der = public_key.public_bytes(
        encoding=serialization.Encoding.DER,
        format=serialization.PublicFormat.SubjectPublicKeyInfo
    )
    public_key_hash = hashlib.sha256(public_der).digest()
    return public_key_hash


def calculate_offsets():
    feature_offset = bl2_offset
    efuse_offset = feature_offset + bl2_feature_size
    ddr_input_offset = efuse_offset + efuse_cfg_size
    socid_offset = ddr_input_offset + ddr_input_size

    return feature_offset, efuse_offset, ddr_input_offset, socid_offset


def calculate_checksum(data):
    checksum = sum(data) % (2**32)
    return checksum


def convert_en(value):
    if value.lower() == 'true' or value.lower() == 'enable':
        return 1
    elif value.lower() == 'false' or value.lower() == 'disable':
        return 0
    else:
        raise ValueError(
            "Invalid value for wdt_en. Use 'true/enable' or 'false/disable'")


def convert_pin_sub(value):
    if value.lower() == 'none':
        return 0
    elif value.lower() == 'aon':
        return 1
    elif value.lower() == 'hsio':
        return 2
    elif value.lower() == 'display':
        return 3
    elif value.lower() == 'lsio':
        return 4
    elif value.lower() == 'dsp':
        return 5
    else:
        raise ValueError(f"Invalid value: '{value}' for pin_sub."
                         "Use 'aon', 'hsio', 'display', 'lsio' or 'dsp'")


def convert_polarity(value):
    if value.lower() == 'default_low':
        return 0
    elif value.lower() == 'default_high':
        return 1
    else:
        raise ValueError("Invalid value for polarity."
                         "Use 'default_low' or 'default_hign'")


def convert_ddr_type(value):
    print("value:", value)
    if value.lower() == 'lpddr4':
        return 1
    elif value.lower() == 'lpddr4x':
        return 2
    else:
        raise ValueError("Invalid value for ddr type."
                         "Use 'lpddr4' or 'lpddr4x'")


def convert_rank_type(value):
    if value.lower() == 'single':
        return 1
    elif value.lower() == 'dual':
        return 2
    else:
        raise ValueError("Invalid value for ddr rank type."
                         "Use 'single' or 'dual'")


def get_default_gpio_info():
    gpio_sub = convert_pin_sub('aon')
    gpio_group = 0
    gpio_num = 7
    gpio_polarity = 0

    gpio_info = struct.pack('BBBB', gpio_sub, gpio_group,
                            gpio_num, gpio_polarity)
    return gpio_info


def get_gpio_info(gpio):
    gpio_sub = convert_pin_sub(gpio['gpio_sub'])
    gpio_group = gpio['gpio_group']
    gpio_num = gpio['gpio_num']
    if 'polarity' in gpio:
        gpio_polarity = convert_polarity(gpio['polarity'])
    else:
        gpio_polarity = 0
    gpio_info = struct.pack('BBBB', gpio_sub, gpio_group,
                            gpio_num, gpio_polarity)
    return gpio_info


def get_bl2_gpio_info(gpio):
    res = b'\0'
    count = 0

    if not gpio:
        return res
    for k, v in gpio.items():
        k = k.split('_')
        sub = convert_pin_sub(k[0])
        group = int(k[1])
        if '-' not in k[2]:
            nums = [int(k[2])]
        else:
            nums = range(int(k[2].split('-')[0]),
                         int(k[2].split('-')[1]) + 1, 1)
        for num in nums:
            count += 1
            if res == b'\0':
                res = struct.pack('4B', sub, group, num, v['value'])
            else:
                res += struct.pack('4B', sub, group, num, v['value'])
            if count > 40:
                raise ValueError("The number of gpio exceeds the limit, "
                                 "with a maximum of 40")

    return res


def get_ddr_info(data):
    ddr_adc_en = None
    ddr_type = None
    rank_type = None
    freq_data = None

    ddr_adc_en = not convert_en(
        data['bl2_cfg']['ddr']['force']['force_enable'])
    ddr_type = convert_ddr_type(data['bl2_cfg']['ddr']['force']['ddr_type'])

    rank_type = convert_rank_type(data['bl2_cfg']['ddr']['force']['rank_type'])

    ecc_enabled = convert_en(data['bl2_cfg']['ddr']['ecc_enabled'])

    freq_data = data['bl2_cfg']['ddr']['freq']
    if isinstance(freq_data, str):
        if freq_data == "default":
            ddr_freq = 0
        else:
            print("invalid freq value:", freq_data,
                  "valid freq value: string default or int freq data")
            sys.exit(1)
    elif isinstance(freq_data, int):
        ddr_freq = freq_data
    else:
        print("invalid freq value:", freq_data,
              "valid freq value: string default or int freq data")
        sys.exit(1)

    if ddr_adc_en == 1:
        hb_ddr_attr = os.getenv('HR_DDR_ATTR')
        if hb_ddr_attr is not None:
            if hb_ddr_attr.lower() == '2gddr':
                ddr_adc_en = 0
                ddr_type = 1
                rank_type = 1
                ddr_freq = 3200
                ecc_enabled = 0
            elif hb_ddr_attr.lower() == '4gddr':
                ddr_adc_en = 0
                ddr_type = 1
                rank_type = 2
                ddr_freq = 3733
                ecc_enabled = 0

    return ddr_adc_en, ddr_type, rank_type, ddr_freq, ecc_enabled


def generate_binary_from_json(input_file, key_file, user_rot_key_file,
                              output_file):
    if not os.path.exists(input_file):
        print(f"Input file '{input_file}' does not exist.")
        sys.exit(1)

    with open(input_file, 'r') as file:
        data = json.load(file)

    feature_offset, efuse_offset, ddr_input_offset, socid_offset = \
        calculate_offsets()

    wdt_en = convert_en(data['bl2_cfg']['feature']['wdt_en'])

    gpio_cfg = get_bl2_gpio_info(data['bl2_cfg']['feature']['gpio_cfg'])

    sec_en = convert_en(data['bl2_cfg']['efuse_cfg']['secure_boot'])

    burn_user_rot_key_en = convert_en(
        data['bl2_cfg']['efuse_cfg']['burn_user_rot_key'])

    disable_debug = convert_en(data['bl2_cfg']['efuse_cfg']['debug_disable'])

    if data['bl2_cfg']['efuse_cfg'].get('power_gpio', None):
        power_gpio = struct.unpack('BBBB', get_gpio_info(data['bl2_cfg']
                                                         ['efuse_cfg']
                                                         ['power_gpio']))
    else:
        power_gpio = struct.unpack('BBBB', get_default_gpio_info())
    status_gpio = struct.unpack('BBBB', get_gpio_info(data['bl2_cfg']
                                                          ['efuse_cfg']
                                                          ['status_gpio']))
    delay_before = data['bl2_cfg']['efuse_cfg']['delay_before_efuse']
    delay_after = data['bl2_cfg']['efuse_cfg']['delay_after_efuse']

    key_hash = calculate_public_key_hash(key_file)
    if sec_en == 1:
        key_hash = calculate_public_key_hash(key_file)
    else:
        key_hash = b'\x00' * 32

    if burn_user_rot_key_en == 1:
        with open(user_rot_key_file, "rb") as f:
            user_rot_key = f.read()
    else:
        user_rot_key = b'\x00' * 16

    key_hash_values = struct.unpack("<IIIIIIII", key_hash)

    ddr_adc_en, ddr_type, rank_type, ddr_freq, ecc_enabled = get_ddr_info(data)

    bl2_cfg_data = struct.pack("<QIIIIIIIIIQII160sQIIIIIIIIIII16sBBBBBBBBIi"
                               "6I",
                               int.from_bytes(b"HBBL2CFG", byteorder='little'),
                               0,  # Placeholder for checksum
                               feature_offset,
                               bl2_feature_size,
                               efuse_offset,
                               efuse_cfg_size,
                               ddr_input_offset,
                               ddr_input_size,
                               socid_offset,
                               socid_size,
                               int.from_bytes(b"BL2-FEAT", byteorder='little'),
                               wdt_en,
                               data['bl2_cfg']['feature']['wtd_timeout'],
                               gpio_cfg,
                               int.from_bytes(b"HB-EFUSE", byteorder='little'),
                               data['bl2_cfg']['efuse_cfg']['bypass'],
                               sec_en | (disable_debug << 1),
                               1,
                               *key_hash_values,
                               user_rot_key,
                               *power_gpio,
                               *status_gpio,
                               delay_before,
                               delay_after,
                               ddr_adc_en,
                               data['bl2_cfg']['ddr']['detect']['adc_channel'],
                               ddr_type,
                               rank_type,
                               ddr_freq,
                               ecc_enabled)

    checksum = calculate_checksum(bl2_cfg_data)
    socid_file = "tmp_socid.bin"
    checksum += parse_socid(input_file, socid_file)
    bl2_cfg_data = struct.pack("<QIIIIIIIIIQII160sQIIIIIIIIIII16sBBBBBBBBIi"
                               "6I",
                               int.from_bytes(b"HBBL2CFG", byteorder='little'),
                               checksum,  # Placeholder for checksum
                               feature_offset,
                               bl2_feature_size,
                               efuse_offset,
                               efuse_cfg_size,
                               ddr_input_offset,
                               ddr_input_size,
                               socid_offset,
                               socid_size,
                               int.from_bytes(b"BL2-FEAT", byteorder='little'),
                               wdt_en,
                               data['bl2_cfg']['feature']['wtd_timeout'],
                               gpio_cfg,
                               int.from_bytes(b"HB-EFUSE", byteorder='little'),
                               data['bl2_cfg']['efuse_cfg']['bypass'],
                               sec_en | (disable_debug << 1),
                               1,
                               *key_hash_values,
                               user_rot_key,
                               *power_gpio,
                               *status_gpio,
                               delay_before,
                               delay_after,
                               ddr_adc_en,
                               data['bl2_cfg']['ddr']['detect']['adc_channel'],
                               ddr_type,
                               rank_type,
                               ddr_freq,
                               ecc_enabled)

    with open(output_file, 'wb') as output_file_part:
        output_file_part.write(bl2_cfg_data)
        print(f"Binary file generated: {output_file_part.name}")
    with open(socid_file, 'rb') as socid_file:
        data_to_append = socid_file.read()
    with open(output_file, 'ab+') as output_file_all:
        output_file_all.write(data_to_append)
        os.remove("tmp_socid.bin")


if __name__ == "__main__":
    if len(sys.argv) != 5:
        print("Usage: python script.py input_file.json \
bl2_pub_key user_root.key output_file.bin")
        sys.exit(1)

    input_file = sys.argv[1]
    key_file = sys.argv[2]
    user_rot_key_file = sys.argv[3]
    output_file = sys.argv[4]
    if not os.path.isfile(input_file):
        print("input json: {input_file} is not exist")
        sys.exit(1)
    if not os.path.isfile(key_file):
        print("key file: {key_file} is not exist")
        sys.exit(1)
    if not os.path.isfile(user_rot_key_file):
        print("key file: {user_rot_key_file} is not exist")
        sys.exit(1)
    generate_binary_from_json(input_file, key_file,
                              user_rot_key_file, output_file)
