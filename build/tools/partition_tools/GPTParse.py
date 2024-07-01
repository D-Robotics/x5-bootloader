#!/usr/bin/env python3
import getopt
import os
import sys
import json
import logging
import copy


def usage():
    print("Usage: GPTParse.py -s <parameters>")
    print("GPTParse.py -s <part_name:attribute>")
    print("part_name=uboot, boot, system, all ... etc.")
    print("attribute=start, end, size, all etc.")
    print("(e.g.) print the size of uboot, run \"GPTParse.py -s uboot:size\"")


def trans_unit(arg, unit, blk_sz):
    arg.lower()
    unit.lower()
    if unit == "b":
        if arg.isdigit():
            return int(arg)
        if not arg[:-1].isdigit():
            print("Error: Unknown size {}".format(arg))
            sys.exit()
        size = int(arg[:-1])
        if arg.endswith("k"):
            return size * 1024
        if arg.endswith("m"):
            return size * 1024 * 1024
        if arg.endswith("g"):
            return size * 1024 * 1024 * 1024
        if arg.endswith("s"):
            return size * blk_sz
    if unit == "s":
        if arg.isdigit():
            return int(arg) // blk_sz
        if not arg[:-1].isdigit():
            print("Error: Unknown size {}".format(arg))
            sys.exit()
        size = int(arg[:-1])
        if arg.endswith("k"):
            return size * 1024 // blk_sz
        if arg.endswith("m"):
            return size * 1024 * 1024 // blk_sz
        if arg.endswith("g"):
            return size * 1024 * 1024 * 1024 // blk_sz
        if arg.endswith("s"):
            return size * 512 // blk_sz
    elif unit == "k":
        if arg.isdigit():
            return int(arg) // 1024
        if not arg[:-1].isdigit():
            print("Error: Unknown size {}".format(arg))
            sys.exit()
        size = int(arg[:-1])
        if arg.endswith("k"):
            return size
        if arg.endswith("m"):
            return size * 1024
        if arg.endswith("g"):
            return size * 1024 * 1024
        if arg.endswith("s"):
            return size * blk_sz // 1024
    elif unit == "m":
        if arg.isdigit():
            return int(arg) // 1024 // 1024
        if not arg[:-1].isdigit():
            print("Error: Unknown size {}".format(arg))
            sys.exit()
        size = int(arg[:-1])
        if arg.endswith("k"):
            return size // 1024
        if arg.endswith("m"):
            return size
        if arg.endswith("g"):
            return size * 1024
        if arg.endswith("s"):
            return size * blk_sz // 1024 // 1024

    print("Error: Unknown size {}".format(arg))
    sys.exit(-1)


class env_setup:
    """
    @description: export envrionment var
    ---------
    @param: None
    -------
    @Returns: None
    -------
    """

    def __init__(self) -> None:
        self.blk_sz = int(os.getenv('BLK_SZ', 512))
        self.mmc_ufs_erase_size = int(os.getenv('MMC_UFS_ERASE_SIZE', 524288))
        self.nor_erase_size = int(os.getenv('NOR_ERASE_SIZE', 32768))
        self.nand_erase_size = int(os.getenv('NAND_ERASE_SIZE', 131072))
        self.hyper_erase_size = int(os.getenv('HYPER_ERASE_SIZE', 262144))
        self.rootfs_dir = os.getenv('HR_TARGET_DEPLOY_DIR') + "/system"
        self.gpt_config = os.getenv(
            'HR_PART_CONF_FILENAME', './x5-soc-debug-gpt.json')
        self.out_gpt_config = os.getenv(
            'HR_TARGET_PRODUCT_DIR', './out') + "/" + os.path.basename(self.gpt_config)


g_env = env_setup()


LINUX_FS_TYPE = ['ext4', 'ext3', 'ext2', 'yaffs2', 'ubifs', 'jffs2']
MS_FS_TYPE = ['vfat', 'ntfs', 'fat32', 'exfat']


def assign_part_type_guid(fstype):
    if (fstype in LINUX_FS_TYPE):
        # Use "Linux Filesystem" as default GUID for fs in LINUX_FS_TYPE
        part_type_guid = '0FC63DAF-8483-4772-8E79-3D69D8477DE4'
    elif (fstype in MS_FS_TYPE):
        # Use "Microsoft basic data" as default GUID for fs in MS_FS_TYPE
        part_type_guid = 'EBD0A0A2-B9E5-4433-87C0-68B6B72699C7'
    else:
        part_type_guid = ''

    return part_type_guid


class Partition():
    def __init__(self, part_name, part_conf, part_num, cur_start) -> None:
        self.base_name = None
        self.part_name = part_name
        self.part_num = part_num
        self.components = []
        self.depends = []
        self.is_rootfs = False
        self.fstype = None
        self.part_type = "PERM"
        self.part_type_guid = None
        self.pre_cmd = None
        self.post_cmd = None
        self.size = None
        self.start = cur_start
        self.end = None
        self.ota_update_mode = None
        self.ota_is_update = None
        self.medium = "emmc"
        self.magic = None
        self.have_anti_ver = None
        self.com_v2 = {}
        self._parse(part_conf)

    def _parse(self, part_conf):
        image_sum = 0
        if part_conf.get('components', None):
            for image in part_conf['components']:
                image_size = image.split(':')[1]
                image_size = trans_unit(image_size, 'b', g_env.blk_sz)
                image = image.split(':')[0] + ":" + str(image_size)
                image_sum += image_size
                self.components.append(image)

        self.depends = part_conf.get('depends', self.depends)
        self.base_name = part_conf.get('base_name', self.base_name)
        self.pre_cmd = part_conf.get('pre_cmd', self.pre_cmd)
        self.post_cmd = part_conf.get('post_cmd', self.post_cmd)
        self.fstype = part_conf.get('fs_type', self.fstype)
        self.is_rootfs = part_conf.get('is_rootfs', self.is_rootfs)
        self.medium = part_conf.get('medium', self.medium)
        self.ota_is_update = part_conf.get('ota_is_update', self.ota_is_update)
        self.ota_update_mode = part_conf.get(
            'ota_update_mode', self.ota_update_mode)
        self.part_type = part_conf.get('part_type', self.part_type)
        self.part_type_guid = assign_part_type_guid(self.fstype)
        self.part_type_guid = part_conf.get(
            'part_type_guid', self.part_type_guid)
        self.magic = part_conf.get('magic', self.magic)
        self.have_anti_ver = part_conf.get('have_anti_ver', self.have_anti_ver)

        for k, v in part_conf.items():
            if isinstance(v, dict):
                self.com_v2[k] = copy.deepcopy(v)

                self.com_v2[k]['size'] = trans_unit(v['size'],
                                                    'b',
                                                    g_env.blk_sz)
                image_sum += self.com_v2[k]['size']

        if part_conf.get('size', None):
            self.end, self.size = sz_to_start_end(
                self.start, part_conf['size'])
        if image_sum > self.size:
            logging.error(
                "Image size exceeds partition limit in {}"
                .format(self.part_name))
            sys.exit(1)


def convertJSON(partition_list) -> list:
    partitions_json = {}
    for partition in partition_list:
        part_conf = copy.deepcopy(partition.com_v2)
        part_conf['depends'] = []
        part_conf['components'] = partition.components
        if len(partition.depends) != 0:
            for depend in partition.depends:
                if partition.part_type == "AB":
                    depend += '_' + partition.part_name.split('_')[-1]
                part_conf['depends'].append(depend)
        part_conf['base_name'] = partition.base_name
        part_conf['part_num'] = partition.part_num
        part_conf['start'] = partition.start
        part_conf['end'] = partition.end
        part_conf['size'] = partition.size
        part_conf['medium'] = partition.medium
        part_conf['fs_type'] = partition.fstype
        part_conf['part_type'] = partition.part_type
        part_conf['is_rootfs'] = partition.is_rootfs
        part_conf['part_type_guid'] = partition.part_type_guid
        part_conf['pre_cmd'] = partition.pre_cmd
        part_conf['post_cmd'] = partition.post_cmd
        part_conf['ota_update_mode'] = partition.ota_update_mode
        part_conf['ota_is_update'] = partition.ota_is_update
        part_conf['magic'] = partition.magic
        part_conf['have_anti_ver'] = partition.have_anti_ver

        partitions_json[partition.part_name] = part_conf

    return partitions_json


def get_sys_ver() -> str:
    ver_file = g_env.rootfs_dir + "/etc/version"
    if not os.path.isfile(ver_file):
        return ""
    ver = None
    with open(ver_file, 'r') as f:
        ver = f.readline()
    return ver.strip()


class Partition_Table():
    def __init__(self) -> None:
        self.nor_part_num = 1
        self.nand_part_num = 1
        self.emmc_part_num = 1
        self.nor_cur_start = 0
        self.nand_cur_start = 0
        self.emmc_cur_start = 0
        self.nor_align = g_env.nor_erase_size
        self.nand_align = g_env.nand_erase_size
        self.emmc_align = g_env.mmc_ufs_erase_size
        self.nor_partitions_list = []
        self.nand_partitions_list = []
        self.emmc_partitions_list = []
        self.part_global = None
        self.backup_slot_count = None
        self.AB_part_a = None
        self.AB_part_b = None
        self.BAK_part_bak = None
        self.backup_dir = None

    def append_global_config(self, global_conf):
        self.part_global = {
            "backup_slot_count": 2,
            "backup_dir": "/userdata/ota/backup_dir",
            "AB_part_a": "_a",
            "AB_part_b": "_b",
            "BAK_part_bak": "_bak",
            "sys_version": f"{get_sys_ver()}",
        }
        self.backup_slot_count = global_conf.get('backup_slot_count',
                                                 self.part_global[
                                                     'backup_slot_count'])
        self.AB_part_a = global_conf.get('AB_part_a',
                                         self.part_global[
                                             'AB_part_a'])
        self.AB_part_b = global_conf.get('AB_part_b',
                                         self.part_global[
                                             'AB_part_b'])
        self.BAK_part_bak = global_conf.get('BAK_part_bak',
                                            self.part_global[
                                                'BAK_part_bak'])

    def _append_part(self, part_name, part_conf) -> None:
        if part_conf.get("medium", None) and part_conf['medium'] == "emmc":
            self._append_emmc(part_name, part_conf)
        elif part_conf.get("medium", None) and part_conf['medium'] == "nor":
            self._append_nor(part_name, part_conf)
        elif part_conf.get("medium", None) and part_conf['medium'] == "nand":
            self._append_nand(part_name, part_conf)
        elif part_conf.get("medium") is None:
            part_conf['medium'] = "emmc"
            self._append_emmc(part_name, part_conf)
        else:
            logging.error("{} is not supported on partition {}".format(
                part_conf['medium'], part_name))
            sys.exit(1)

    def append_part(self, part_name, part_conf) -> None:
        part_conf['base_name'] = part_name
        if part_conf.get("part_type", None) and part_conf['part_type'] == "AB":
            part_name_A = part_name + self.AB_part_a
            self._append_part(part_name_A, part_conf)
            part_name_B = part_name + self.AB_part_b
            self._append_part(part_name_B, part_conf)
        elif (part_conf.get("part_type", None) and
                part_conf['part_type'] == "BAK"):
            self._append_part(part_name, part_conf)
            for i in range(1, self.backup_slot_count):
                bak_part_name = part_name + self.BAK_part_bak + str(i)
                self._append_part(bak_part_name, part_conf)
        else:
            self._append_part(part_name, part_conf)

    def _append_nor(self, part_name, part_conf):
        self.nor_partitions_list.append(
            Partition(part_name, part_conf, self.nor_part_num,
                      self.nor_cur_start))
        part_size = self.nor_partitions_list[-1].size
        if (part_size % self.nor_align != 0):
            logging.error(f"NOR partitions must be {self.nor_align // 1024}k \
                aligned, current {part_name} partition size is {part_size}"
                          .replace('  ', ''))
            sys.exit(1)
        self.nor_part_num += 1
        self.nor_cur_start += part_size

    def _append_nand(self, part_name, part_conf):
        self.nand_partitions_list.append(
            Partition(part_name, part_conf, self.nand_part_num,
                      self.nand_cur_start))
        part_size = self.nand_partitions_list[-1].size
        if (part_size % self.nand_align != 0):
            logging.error(f"NAND partitions must be {self.nand_align // 1024}k \
                aligned, current {part_name} partition size is {part_size}"
                          .replace('  ', ''))
            sys.exit(1)
        self.nand_part_num += 1
        self.nand_cur_start += part_size

    def _append_emmc(self, part_name, part_conf):
        self.emmc_partitions_list.append(
            Partition(part_name, part_conf, self.emmc_part_num,
                      self.emmc_cur_start))
        part_size = self.emmc_partitions_list[-1].size
        if (part_size % g_env.blk_sz != 0):
            logging.error(f"EMMC partitions must be {g_env.blk_sz} bytes\
                 aligned, current {part_name} partition size is {part_size}"
                          .replace('  ', ''))
            sys.exit(1)
        self.emmc_part_num += 1
        self.emmc_cur_start += part_size


def sz_to_start_end(start, _size) -> int:
    """
    @description: calculator 'start' and 'end', uint is sector
    ---------
    @param:
        prev_end: End of the previous partition
        __size: Size of the current partition With unit
    -------
    @Returns: the start end size of the current partition
    -------
    """
    size = trans_unit(_size, 'b', g_env.blk_sz)
    end = start + size - 1

    return end, size


def _parse(_conf_d, gpt_path) -> dict:
    """
    @description: Parse partition config JSON file,
        add 'start'/'end'/'size'/'part_type'/'mounttype'.
        Then it is saved as json in the out directory.
    ---------
    @param: JSON data obtained from partition JSON
    -------
    @Returns: Parsed JSON data
    -------
    """
    parttion_table = Partition_Table()
    conf_d = {}
    parttion_table.append_global_config(_conf_d.get("global", {}))
    for key, attr in _conf_d.items():
        if key == "global":
            continue
        part_name = key
        if "PA_" in key or "PB_" in key:
            key = key[3:]
        if isinstance(attr, dict):
            parttion_table.append_part(part_name, attr)
        else:
            part_config_file = gpt_path + "/" + attr
            try:
                with open(part_config_file, "r") as f:
                    part_all_conf = json.load(f)
                if not part_all_conf.get(key, None):
                    raise OSError("config {} not in {}".format(
                        key, part_config_file))
                parttion_table.append_part(part_name, part_all_conf[key])
            except OSError as e:
                logging.error(e)

    if parttion_table.part_global:
        conf_d["global"] = parttion_table.part_global
    if parttion_table.nor_partitions_list:
        conf_d["nor"] = convertJSON(parttion_table.nor_partitions_list)
    if parttion_table.nand_partitions_list:
        conf_d["nand"] = convertJSON(parttion_table.nand_partitions_list)
    if parttion_table.emmc_partitions_list:
        conf_d["emmc"] = convertJSON(parttion_table.emmc_partitions_list)

    return conf_d


def parse_conf(config_path) -> dict:
    """
    @description: Determine which of the parsed JSON and
        the original JSON is updated, and return the latest JSON data
    ---------
    @param: partition table path
    -------
    @Returns: Parsed JSON data
    -------
    """
    with open(config_path, 'r') as ff:
        conf_d = json.load(ff)

    mconf = _parse(conf_d, os.path.dirname(config_path))

    json_data = json.dumps(mconf, indent=4, separators=(',', ': '))
    if not os.path.exists(os.path.dirname(g_env.out_gpt_config)):
        os.makedirs(os.path.dirname(g_env.out_gpt_config))
    with open(g_env.out_gpt_config, 'w') as f:
        f.write(json_data)
    return mconf


def search_part(part, attribute) -> any:
    """
    @description: search partition attribute
    ---------
    @param:
        part: partition name,
        attribute: partition attribute
    -------
    @Returns: partition attribute value
    -------
    """
    result = None
    is_found = False
    conf_d = parse_conf(g_env.gpt_config)
    part_a = f"{part}{conf_d['global']['AB_part_a']}"

    for medium, v in conf_d.items():
        if v.get(part, None):
            result = v[part][attribute]
            is_found = True
            break
        elif v.get(part_a, None):
            result = v[part_a][attribute]
            is_found = True
            break
        else:
            for n, attr in v.items():
                if isinstance(attr, dict):
                    if attr.get(part, None):
                        result = attr[part][attribute]
                        is_found = True
                        break

    if not is_found:
        logging.error(part + " is not in " + g_env.gpt_config)
        sys.exit(-1)

    if attribute == "components":
        for res in result:
            print(res.split(':')[0])
    else:
        print(result)
    return result


def get_mtd_parts():
    conf_d = parse_conf(g_env.gpt_config)
    mtd_ids = "spi7.0"
    if conf_d.get("nor", None):
        p_conf = conf_d['nor']
    elif conf_d.get("nand", None):
        p_conf = conf_d['nand']
    else:
        return None

    mtd_parts = mtd_ids + ":"
    for part_name, part_conf in p_conf.items():
        if part_name == "userdata":
            mtd_parts += f"-@{hex(part_conf['start'])}({part_name})"
        else:
            mtd_parts += \
                f"{part_conf['size']}@{hex(part_conf['start'])}({part_name}),"
    if mtd_parts.endswith(","):
        mtd_parts += mtd_parts[:-1]
    print(mtd_parts)


def get_part_list():
    conf_d = parse_conf(g_env.gpt_config)
    if conf_d.get("nor", None):
        part_names = list(conf_d['nor'].keys())
    elif conf_d.get("nand", None):
        part_names = list(conf_d['nand'].keys())
    elif conf_d.get("emmc", None):
        part_names = list(conf_d['emmc'].keys())
    else:
        logging.error(
            "Unable to get partition table name list from " + g_env.gpt_config)
        sys.exit(-1)

    formatted_part_names = ' '.join(part_names)
    print(formatted_part_names)
    return part_names


def get_miniboot_list():
    part_names = []
    conf_d = parse_conf(g_env.gpt_config)
    if conf_d.get("nor", None) and conf_d['nor'].get("miniboot", None):
        miniboot_attr = conf_d['nor']['miniboot']
    elif conf_d.get("nand", None) and conf_d['nand'].get("miniboot", None):
        miniboot_attr = conf_d['nand']['miniboot']
    elif conf_d.get("emmc", None) and conf_d['emmc'].get("miniboot", None):
        miniboot_attr = conf_d['emmc']['miniboot']
    else:
        logging.error(
            "Unable to get partition table name list from " + g_env.gpt_config)
        sys.exit(-1)

    for k, v in miniboot_attr.items():
        if isinstance(v, dict):
            part_names.append(k)

    formatted_part_names = ' '.join(part_names)
    print(formatted_part_names)
    return part_names


def main(argv):
    try:
        opts, args = getopt.getopt(argv[1:], "lhs:pgm", ["help"])
    except getopt.GetoptError:
        usage()
        sys.exit(1)
    for opt, arg in opts:
        if opt in ("-h", "--help"):
            usage()
            sys.exit(0)
        elif opt == "-l":
            get_part_list()
        elif opt == "-g":
            get_miniboot_list()
        elif opt == "-s":
            arg = arg.split(':')
            if len(arg) != 2:
                usage()
                sys.exit(1)
            search_part(arg[0], arg[1])
        elif opt == "-p":
            parse_conf(g_env.gpt_config)
        elif opt == "-m":
            get_mtd_parts()
        else:
            usage()
            sys.exit(1)


if __name__ == '__main__':
    logging.basicConfig(format='{}:%(levelname)s:%(message)s'.format(
        sys.argv[0]), level='ERROR')
    main(sys.argv)
