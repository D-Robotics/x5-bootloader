#!/usr/bin/env python3
import struct
import sys
import logging
import argparse
import os
import traceback

from GPTParse import parse_conf


def addCheckSum(data):
    sum = 0
    for c in data:
        if isinstance(c, int):
            sum += c
        else:
            sum += ord(c)
    return sum


def part_file_parse(partition_file):
    if not os.path.isfile(partition_file):
        print(f"partition file {partition_file} is not exists")
    return parse_conf(partition_file)


class X5_MBR():
    MAGIC = 0x46495041
    RESERVED = 115 * 4
    MBR_FORMAT = ('<I'  # magic number
                  'I'   # nor flash cfg addr
                  'I'   # bl2 fip main addr
                  'I'   # bl2 fip bak1 addr
                  'I'   # bl2 fip bak2 addr
                  'I'   # bl2 fip bak3 addr
                  'I'   # bl3x fip a addr
                  'I'   # bl3x fip b addr
                  'I'   # misc addr
                  'I'   # uboot_a addr
                  'I'   # uboot_b addr
                  'I'   # veeprom addr
                  f'{RESERVED}s')   # reserved

    def __init__(self, _part_conf):
        new_offset = 0
        if _part_conf.get('nor', None):
            part_conf = _part_conf['nor']
        elif _part_conf.get('nand', None):
            part_conf = _part_conf['nand']
        elif _part_conf.get('emmc', None):
            part_conf = _part_conf['emmc']
            new_offset = 20 * 1024

        self.nor_cfg_addr = part_conf['norcfg']['start'] - new_offset \
            if part_conf.get('norcfg', None) else 0
        self.bl2_main_addr = part_conf['miniboot']['start'] - new_offset \
            if part_conf.get('miniboot', None) else 0
        self.bl2_bak1_addr = part_conf['miniboot_bak1']['start'] - new_offset \
            if part_conf.get('miniboot_bak1', None) else 0
        self.bl2_bak2_addr = part_conf['bl2_bak2']['start'] - new_offset \
            if part_conf.get('bl2_bak2', None) else 0
        self.bl2_bak3_addr = part_conf['bl2_bak3']['start'] - new_offset \
            if part_conf.get('bl2_bak3', None) else 0
        self.bl3x_a_addr = self.bl2_main_addr + \
            part_conf['miniboot']['bl2']['size'] \
            if part_conf.get('miniboot') and part_conf['miniboot'].get('bl2') \
            else 0
        self.bl3x_b_addr = self.bl2_bak1_addr + \
            part_conf['miniboot_bak1']['bl2']['size'] \
            if part_conf.get('miniboot_bak1') and \
            part_conf['miniboot_bak1'].get('bl2') else 0
        self.misc_addr = part_conf['misc']['start'] - new_offset \
            if part_conf.get('misc', None) else 0
        if part_conf.get('uboot', None):
            self.uboot_a_addr = part_conf['uboot']['start'] - new_offset \
                if part_conf.get('uboot', None) else 0
            self.uboot_b_addr = part_conf['uboot_bak1']['start'] - new_offset \
                if part_conf.get('uboot_bak1', None) else 0
        else:
            self.uboot_a_addr = part_conf['uboot_a']['start'] - new_offset \
                if part_conf.get('uboot_a', None) else 0
            self.uboot_b_addr = part_conf['uboot_b']['start'] - new_offset \
                if part_conf.get('uboot_b', None) else 0
        self.veeprom_addr = part_conf['ubootenv']['start'] - new_offset + \
            192 * 1024 if part_conf.get('ubootenv', None) else 0

    def to_img(self):
        mbr_body = struct.pack(self.MBR_FORMAT,
                               self.MAGIC,
                               self.nor_cfg_addr,
                               self.bl2_main_addr,
                               self.bl2_bak1_addr,
                               self.bl2_bak2_addr,
                               self.bl2_bak3_addr,
                               self.bl3x_a_addr,
                               self.bl3x_b_addr,
                               self.misc_addr,
                               self.uboot_a_addr,
                               self.uboot_b_addr,
                               self.veeprom_addr,
                               b'\0' * self.RESERVED)
        body_size = struct.calcsize(self.MBR_FORMAT)
        image = struct.pack(f'<{body_size}sI',
                            mbr_body,
                            addCheckSum(mbr_body))
        return image


class HBMbr():
    def make_mbr(self, chip, part_conf, output):
        if chip == "x5":
            hb_mbr = X5_MBR(part_conf)
        mbr_img = output + "/mbr.img"
        with open(mbr_img, "wb") as f:
            f.write(hb_mbr.to_img())
        logging.info("mbr image make success")


class HBMbrTool(object):

    def __init__(self) -> None:
        self.hb_img = HBMbr()

    def run(self, argv):
        parser = argparse.ArgumentParser(description='HB mbr generate')
        subparsers = parser.add_subparsers(title='subcommands')

        sub_parser = subparsers.add_parser('make_mbr',
                                           help="make mbr image")
        sub_parser.add_argument('--partition_file',
                                help='Partition table file')
        sub_parser.add_argument('--chip',
                                help='Chip')
        sub_parser.add_argument('--output',
                                help='Output dir')
        sub_parser.set_defaults(func=self.make_mbr)

        args = parser.parse_args(argv[1:])
        try:
            args.func(args)
        except Exception as e:
            sys.stderr.write('{}: {}\n'.format(argv[0], e))
            traceback.print_exc()
            sys.exit(1)

    def make_mbr(self, args):
        part_conf = part_file_parse(args.partition_file)
        self.hb_img.make_mbr(args.chip, part_conf, args.output)


if __name__ == '__main__':
    logging.basicConfig(level=logging.INFO,
                        # filename=os.getenv('OUTPUT_LOG_DIR') +
                        # "/package.log",
                        # filemode="a",
                        format="%(asctime)s - %(filename)s - %(funcName)s \
- %(levelname)s - %(message)s")
    tool = HBMbrTool()
    tool.run(sys.argv)
