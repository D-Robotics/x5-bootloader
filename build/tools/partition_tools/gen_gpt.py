#!/usr/bin/env python3
from multiprocessing.sharedctypes import Value
import sys
import os
import uuid
import logging

from gpt import *
from GPTParse import parse_conf


def create_empty_gpt_entry():
    partition_type_guid = \
        b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    unique_partition_guid = \
        b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    starting_lba = 0
    ending_lba = 0
    attributes = 0
    partition_name = "\x00\x00".encode('utf-16')[2:]
    # print(partition_name)
    # '<16s 16s Q Q Q 72s'
    entry = GPTPartitionEntry(partition_type_guid, unique_partition_guid,
                              starting_lba,
                              ending_lba,
                              attributes,
                              partition_name)
    return entry


def create_gpt_entry(start_lba, end_lba, name, part_type_guid=None):
    if part_type_guid:
        partition_type_guid = part_type_guid
    else:
        partition_type_guid = uuid.uuid4().bytes
    unique_partition_guid = uuid.uuid4().bytes
    starting_lba = start_lba
    ending_lba = end_lba
    attributes = 0
    partition_name = bytes(name, 'UTF-16')[2:]
    entry = GPTPartitionEntry(partition_type_guid,
                              unique_partition_guid,
                              starting_lba,
                              ending_lba,
                              attributes,
                              partition_name)
    return entry


def complete_gpt_entries(entrylist):
    an = create_empty_gpt_entry()
    for i in range(len(entrylist), 0x80, 1):
        entrylist.append(an)


def create_gpt_header():
    signature = b"EFI PART"
    revision = b"\x00\x00\x01\x00"
    header_size = 0x5c
    header_crc32 = 0  # TODO
    reserved = b"\x00\x00\x00\x00"
    my_lba = 1
    alternate_lba = 0  # TODO 15093062  00 E6 4D 46
    first_usable_lba = 34
    last_usable_lba = 0  # TODO
    disk_guid = uuid.uuid4().bytes
    partition_entry_lba = 2
    number_of_partition_entries = 0x80
    size_of_partition_entry = 0x80
    partition_entry_array_crc32 = 0  # TODO
    hdr = GPTHeader(signature, revision, header_size, header_crc32, reserved,
                    my_lba, alternate_lba, first_usable_lba, last_usable_lba,
                    disk_guid, partition_entry_lba,
                    number_of_partition_entries, size_of_partition_entry,
                    partition_entry_array_crc32)

    return hdr


def create(alternate_lba, entrylist):
    complete_gpt_entries(entrylist)

    entriesdata = encode_gpt_partition_entry_array(entrylist, 0x80, 0x80)

    hdr = create_gpt_header()
    hdr.alternate_lba = alternate_lba
    hdr.last_usable_lba = alternate_lba - 33
    hdr.partition_entry_array_crc32 = calculate_partition_entry_array_crc32(
        entriesdata)
    hdr.header_crc32 = hdr.calculate_header_crc32()

    gptdata = encode_gpt_header(hdr)

    # gpt_main
    maindata = bytearray()
    # mbr
    maindata.extend(bytearray(0x1C0))
    maindata.extend(pack('I', 0xFFEE0002))
    maindata.extend(pack('I', 0x0001FFFF))
    maindata.extend(pack('I', 0xFFFF0000))
    maindata.extend(pack('I', 0x0000007F))
    maindata.extend(bytearray(0x2C))
    maindata.extend(pack('I', 0xAA550000))
    # gpt header
    maindata.extend(gptdata)
    maindata.extend(bytearray(0x200 - len(gptdata)))
    # gpt table
    maindata.extend(entriesdata)
    # maindata.extend(bytearray(128 * 128 - len(entriesdata)))

    # gpt_backup
    backdata = bytearray()
    # gpt table
    backdata.extend(entriesdata)
    # gpt header
    backdata.extend(gptdata)
    backdata.extend(bytearray(0x200 - len(gptdata)))

    return (bytes(maindata), bytes(backdata))


def mmc_parse_conf(emmc_conf, entrylist):
    blk_sz = int(os.getenv('BLK_SZ'))
    maxsize = 0

    for name, attr in emmc_conf.items():

        if name == "gpt":
            continue

        entry = create_gpt_entry(int(attr['start'] // blk_sz),
                                 int(attr['end'] // blk_sz),
                                 name,
                                 attr['part_type_guid'])
        entrylist.append(entry)
        maxsize = max(maxsize, int(attr['end']) // blk_sz)

    return int(maxsize) + 33


def creat_gpt_img(emmc_partitions, main_img, backup_img):
    entrylist = []
    alternate_lba = mmc_parse_conf(emmc_partitions, entrylist)
    (main_data, backup_data) = create(alternate_lba, entrylist)

    with open(main_img, "wb") as f:
        f.write(main_data)

    with open(backup_img, "wb") as f:
        f.write(backup_data)

    logging.info(f"{main_img} make success")
    logging.info(f"{backup_img} make success")
    return main_data


if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Error: args number is not 2")
        print(f"Usage: {sys.argv[0]} <gpt_config> <image_out_dir>")
        sys.exit(1)

    cfg_path = sys.argv[1]
    image_out_dir = sys.argv[2]

    if not os.path.isfile(cfg_path):
        print(f"{cfg_path} is not exits")
    if not os.path.exists(image_out_dir):
        print(f"{image_out_dir} is not exits")
    main_img = image_out_dir + "/gpt.img"
    backup_img = image_out_dir + "/gpt_back.img"
    part_conf = parse_conf(cfg_path)
    if part_conf.get("emmc", None):
        creat_gpt_img(part_conf['emmc'], main_img, backup_img)
