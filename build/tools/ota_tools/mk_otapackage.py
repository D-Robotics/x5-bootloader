#!/usr/bin/env python3
import json
import os
import sys
import zipfile
import logging
import argparse
import shutil
import hashlib
import subprocess
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.primitives.serialization import load_pem_private_key


class HBOTAError(Exception):
    """Application-specific errors.

    These errors represent issues for which a stack-trace should not be
    presented.

    Attributes:
      message: Error message.
    """

    def __init__(self, message):
        Exception.__init__(self, message)


def run_shell(cmdline):
    logging.info(' '.join(cmdline))
    done_proc = subprocess.run(
        cmdline, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    logging.info(done_proc.stdout.decode())
    return done_proc


def get_file_md5(file_path, withsign=False):
    buf = open(file_path, 'rb').read()
    if (withsign is True):
        total_len = len(buf)
        chunk_size = 2 * 1024 * 1024
        signature_length = 2 * 1024
        cur_pos = 0
        new_buffer = b''
        while cur_pos < total_len:
            if (total_len - cur_pos) < chunk_size + signature_length:
                data_chunk = buf[cur_pos:-(signature_length)]
                new_buffer += data_chunk
                break
            else:
                data_chunk = buf[cur_pos:cur_pos + chunk_size]
                cur_pos += chunk_size + signature_length
                new_buffer += data_chunk
    else:
        new_buffer = buf

    return hashlib.md5(new_buffer).hexdigest()


def get_old_sys_version(old_dir):
    old_data_file = old_dir + "/data.json"
    if not os.path.isfile(old_data_file):
        print("Invalid package, cannot find data.json")
        sys.exit(-1)
    with open(old_data_file, 'r') as f:
        old_data = json.load(f)
    return old_data['sys_version']


def sigh_pkg(pkg, key_file):
    with open(key_file, "rb") as f:
        key_data = f.read()
        pkey = load_pem_private_key(key_data, password=None)

    with open(pkg, 'rb') as f:
        pkg_data = f.read()

    signature = pkey.sign(pkg_data, padding.PKCS1v15(), hashes.SHA256())
    sigh_file = os.path.join(os.path.dirname(
        pkg), os.path.basename(pkg).split('.')[0] + '.signature')

    with open(sigh_file, "wb") as signature_file:
        signature_file.write(signature)


def compress_zip(out_file, files_list):
    """
    @description: Compress the file to zip
    ---------
    @param:
        out_file: out zip file
        files_list: list of files to be compressed
    -------
    @Returns:
    -------
    """
    with zipfile.ZipFile(out_file, "w", zipfile.ZIP_DEFLATED) as zf:
        if isinstance(files_list, list):
            for _file in files_list:
                if os.path.isfile(_file):
                    zf.write(_file, _file[_file.rfind("/") + 1:])
                elif os.path.isdir(_file):
                    if _file.endswith('/'):
                        base_name = os.path.basename(os.path.dirname(_file))
                    else:
                        base_name = os.path.basename(_file)
                    for root, _, files in os.walk(_file):
                        relative_root = '' if root == _file else root.replace(
                            _file, '') + os.sep
                        relative_root = base_name + '/' + relative_root
                        for filename in files:
                            zf.write(os.path.join(root, filename),
                                     relative_root + filename)
                else:
                    logging.error("compress failed, ", _file, " not found")
                    sys.exit()
        else:
            if not os.path.isfile(files_list):
                logging.error("compress failed, ", files_list, " not found")
                sys.exit()
            zf.write(files_list, files_list[files_list.rfind("/") + 1:])


def uncompress_zip(ori_file, out_dir):
    if not os.path.exists(out_dir):
        os.makedirs(out_dir)
    try:
        with zipfile.ZipFile(ori_file) as zf:
            zf.extractall(out_dir)
    except Exception:
        logging.error(f"pkg {ori_file} uncompress failed")
        sys.exit(-1)


class HBOTAPKG(object):
    def __init__(self, sign_key=None) -> None:
        self.image_dir = None
        self.sign_key = sign_key
        self.inc = False
        self.app_middleware = False
        self.repack = False
        self.ota_sign = False
        self.medium = ['nor', 'emmc']
        self.conf_d = {}
        self.add_file = None
        self.old_dir = None
        self.diffpatch_dir = None

    def ota_unpack(self, ota_pkg, out_dir):
        if not os.path.isfile(ota_pkg):
            logging.error(f"package {ota_pkg} not exits")
            sys.exit(-1)
        uncompress_zip(ota_pkg, out_dir)

    def gen_gpt_conf(self, prepare_dir):
        """
        @description: Generate ota version infornation file and gpt config file
        ---------
        @param: Dictionary after partition table resolution
        -------
        @Returns: None
        -------
        """
        ota_gpt = prepare_dir + "/gpt.conf"
        with open(ota_gpt, 'w') as f:
            for i, m in enumerate(self.medium):
                if not self.conf_d.get(m, None):
                    continue
                for part_name, part_conf in self.conf_d[m].items():
                    if part_name == "gpt":
                        continue
                    f.write(
                        f"{part_name}:{part_conf['start']}:\
{part_conf['end']}:{i}\n")
        return [ota_gpt]

    def get_diff_img(self, old_img, new_img, inc_img):
        diff_tool = self.diffpatch_dir + "/hdiffz"
        patch_tool = self.diffpatch_dir + "/hpatchz"
        if not os.path.isfile(diff_tool) or not os.path.isfile(patch_tool):
            logging.error(
                f"diff_patch tools dir {self.diffpatch_dir} is not exists")
            sys.exit(-1)
        diff_cmd = [diff_tool, old_img, new_img, inc_img, "-s-1k"]
        if run_shell(diff_cmd).returncode != 0:
            print(f"diff {old_img} and {new_img} failed")
            sys.exit(-1)

        check_img = os.path.dirname(
            inc_img) + "/check_" + os.path.basename(old_img)
        if os.path.isfile(check_img):
            os.remove(check_img)
        patch_cmd = [patch_tool, old_img, inc_img, check_img]
        if run_shell(patch_cmd).returncode != 0 or \
                get_file_md5(new_img) != get_file_md5(check_img):
            print(f"patch {old_img} and {inc_img} failed")
            sys.exit(-1)

    def add_part_info(self, part_name, part_conf, prepare_dir):
        image_list = []
        if not part_conf['ota_is_update']:
            return None, None
        if part_conf.get('part_type', None):
            if (part_conf['part_type'] == "AB" and
                    self.conf_d['global']['AB_part_b'] in part_name):
                return None, None
            elif part_conf['part_type'] == "BAK":
                if self.conf_d['global']['BAK_part_bak'][1:] in \
                        part_name.split('_')[-1]:
                    return None, None

        part_info = {}
        part_info["md5sum"] = {}        # [old, new] if inc pkg
        part_info["md5_scope"] = {}     # [old, new] if inc pkg
        part_info["medium"] = part_conf['medium']
        part_info["part_type"] = part_conf['part_type']
        part_info["have_anti_ver"] = part_conf['have_anti_ver']
        if part_conf['ota_update_mode']:
            part_info["upgrade_method"] = part_conf['ota_update_mode']
        else:
            part_info["upgrade_method"] = "image"

        if part_conf.get('raw_img_list', None) and part_conf['raw_img_list']:
            for raw_img in part_conf['raw_img_list']:
                parts = raw_img.split(':')
                raw_img_suffix = parts[2] if len(parts) > 2 else ""
                imgname_mid = (part_conf['base_name'] + "_" +
                               raw_img_suffix)
                if self.ota_sign and part_info["medium"] == 'nor':
                    imgname = imgname_mid + "_signed.img"
                    part_img = (self.image_dir + "ota_signed_img/" +
                                part_conf['base_name'] + "_" +
                                raw_img_suffix + "_signed.img")
                    if not os.path.exists(part_img):
                        part_img = (self.image_dir + "/" +
                                    part_conf['base_name'] + "_" +
                                    raw_img_suffix + "_signed.img")
                    imgsize = os.path.getsize(part_img)
                    imgsize -= ((imgsize // (2*1024*1024 + 2*1024 + 1) + 1)
                                * (2*1024))
                    with_sign = True
                    part_info["imgname"] = (part_conf['base_name']
                                            + "_${board_name}_signed.img")
                else:
                    imgname = imgname_mid + ".img"
                    part_img = (self.image_dir + "/" +
                                part_conf['base_name'] + "_" +
                                raw_img_suffix + ".img")
                    imgsize = os.path.getsize(part_img)
                    part_info["imgname"] = (part_conf['base_name']
                                            + "_${board_name}.img")
                    with_sign = False

                part_info["md5sum"][imgname] = \
                    get_file_md5(part_img, with_sign)
                part_info["md5_scope"][imgname] = imgsize
                image_list.append(part_img)
            return part_info, image_list

        part_img = self.image_dir + "/" + part_conf['base_name'] + ".img"
        if (not os.path.isfile(part_img) and
                part_conf['part_type'] != "PERMANENT"):
            logging.warning(f"{part_img} not exits")
            # TODO tmp code, wait for all image
            return None, None
            # sys.exit(-1)
        if self.inc and part_info["upgrade_method"] == "image_diff":
            part_info["imgname"] = part_conf['base_name'] + '_inc.img'
            old_img = self.old_dir + "/" + part_conf['base_name'] + ".img"
            old_img_name = "old_" + part_conf['base_name'] + '_inc.img'
            if not os.path.isfile(old_img):
                logging.error("Invalid package, cannot find old_img")
                sys.exit(-1)
            part_info["md5sum"][old_img_name] = get_file_md5(old_img)
            part_info["md5_scope"][old_img_name] = \
                os.path.getsize(old_img)
            diff_img = prepare_dir + "/" + part_conf['base_name'] + "_inc.img"
            self.get_diff_img(old_img, part_img, diff_img)
            image_list.append(diff_img)
        elif self.ota_sign and part_info["medium"] == 'nor':
            part_img = (self.image_dir + "ota_signed_img/" +
                        part_conf['base_name'] + "_signed.img")
            if not os.path.exists(part_img):
                part_img = (self.image_dir + "/" + part_conf['base_name']
                            + "_signed.img")
            part_info["imgname"] = part_conf['base_name'] + '_signed.img'
            image_list.append(part_img)
        else:
            part_info["imgname"] = part_conf['base_name'] + '.img'
            image_list.append(part_img)

        imgsize = os.path.getsize(part_img)
        if self.ota_sign and part_info["medium"] == 'nor':
            imgsize -= ((imgsize // (2*1024*1024 + 2*1024 + 1) + 1) *
                        (2*1024))
            with_sign = True
        else:
            with_sign = False
        part_info["md5sum"][part_info["imgname"]] = \
            get_file_md5(part_img, with_sign)
        part_info["md5_scope"][part_info["imgname"]] = imgsize

        return part_info, image_list

    def gen_data_conf_and_add_img(self, prepare_dir):
        """
        @description: Generate warning information file
        ---------
        @param:
            file_path: info file path
            uptime: part upgrade time
        -------
        @Returns: None
        -------
        """
        file_list = []
        data_json = {}
        ota_data = prepare_dir + "/data.json"

        if self.conf_d['global'].get('antirollbackUpdate_host', None):
            data_json['antirollbackUpdate_host'] =\
                self.conf_d['global']['antirollbackUpdate_host']
        else:
            data_json['antirollbackUpdate_host'] = 0

        # data_json['antirollbackUpdate_hsm'] =\
        #     self.conf_d['global']['antirollbackUpdate_hsm']
        if self.conf_d['global'].get('backup_dir', None):
            data_json['backup_dir'] =\
                self.conf_d['global']['backup_dir']
        else:
            data_json['backup_dir'] = 0
        if not self.inc and not self.repack and not self.app_middleware:
            data_json['sys_version'] = self.conf_d['global']['sys_version']
        # data_json['ab_sync'] = self.conf_d['global']['ab_sync']
        data_json['update_partition'] = []
        data_json['nor_sign'] = self.ota_sign

        file_list.append(ota_data)
        partition_info = {}
        for _, m in enumerate(self.medium):
            if not self.conf_d.get(m, None):
                continue
            for part_name, part_conf in self.conf_d[m].items():
                part_info, image_list = self.add_part_info(
                    part_name, part_conf, prepare_dir)
                if part_info:
                    partition_info[part_conf['base_name']] = part_info
                    data_json['update_partition'].append(
                        part_conf['base_name'])
                    file_list += image_list
        data_json['partition_info'] = partition_info
        data_json = json.dumps(data_json, indent=4, separators=(',', ': '))
        with open(ota_data, 'w') as f:
            f.write(data_json)
        return file_list

    def part_file_parse(self, partition_file):
        if not os.path.isfile(partition_file):
            logging.error(f"partition file {partition_file} is not exists")
        with open(partition_file, 'r') as ff:
            self.conf_d = json.load(ff)

    def mk_img(self, dir, out_dir, part_name):
        if not os.path.isdir(dir):
            logging.error(f"{dir} is not exists")
            sys.exit(-1)
        if not os.path.isdir(out_dir):
            os.makedirs(out_dir)
        part_size = 0
        for _, m in enumerate(self.medium):
            for _, part_conf in self.conf_d[m].items():
                if part_conf['base_name'] == part_name:
                    part_size = part_conf['size']
                    break
        if part_size == 0:
            logging.error(f"partition name {part_name} is not exists")
            sys.exit(-1)
        out_img = os.path.join(out_dir, part_name + ".img")
        mk_img_cmd = ['make_ext4fs', '-l', str(part_size), '-L', part_name,
                      out_img, dir]
        if run_shell(mk_img_cmd).returncode != 0:
            logging.error(f"make {out_img} failed")
            sys.exit(-1)
        resize_cmd = ['resize2fs', '-fM', out_img]
        if run_shell(resize_cmd).returncode != 0:
            logging.error(f"resize2fs {out_img} failed")
            sys.exit(-1)

    def handle_app_middleware_img(self, _dir, part_name):
        if not _dir:
            return
        if os.path.isfile(_dir) and _dir.endswith(".img"):
            shutil.copy(_dir, self.image_dir)
        elif os.path.isdir(_dir):
            self.mk_img(_dir, self.image_dir, part_name)
        else:
            logging.error("image or dir error")
            sys.exit(-1)

    def mk_common_pkg(self, prepare_dir, ota_pkg):
        compress_file_list = self.gen_gpt_conf(prepare_dir) + \
            self.gen_data_conf_and_add_img(prepare_dir)

        if not os.path.isfile(self.add_file):
            logging.error("please compile otaupdate code")
            sys.exit(-1)
        compress_file_list.append(self.add_file)

        out_dir = os.path.dirname(ota_pkg)
        if not os.path.exists(out_dir):
            os.makedirs(out_dir)
        if os.path.isfile(ota_pkg):
            os.remove(ota_pkg)

        pkg_info = '\n'
        for c_file in compress_file_list:
            pkg_info += f"\t adding: {os.path.basename(c_file)}\n"
        logging.info(pkg_info)

        compress_zip(ota_pkg, compress_file_list)
        sigh_pkg(ota_pkg, self.sign_key)

    def mk_repack(self, ota_process, prepare_dir, out_dir):
        self.repack = True
        prepare_dir = prepare_dir
        ota_pkg = out_dir + "/all_in_one_repack.zip"
        if os.path.exists(prepare_dir):
            shutil.rmtree(prepare_dir)
        os.makedirs(prepare_dir)
        self.add_file = ota_process
        self.mk_common_pkg(prepare_dir, ota_pkg)

    def mk_sys_pkg(self, ota_process, prepare_dir, out_dir):
        sys_pkg_black_list = []
        prepare_dir = prepare_dir + "/all_in_one/"
        ota_pkg = out_dir + "/all_in_one.zip"
        if os.path.exists(prepare_dir):
            shutil.rmtree(prepare_dir)
        os.makedirs(prepare_dir)
        self.add_file = ota_process
        for _, m in enumerate(self.medium):
            if not self.conf_d.get(m, None):
                continue
            for key, part_conf in self.conf_d[m].items():
                if part_conf['base_name'] in sys_pkg_black_list:
                    self.conf_d[m][key]['ota_is_update'] = False
        self.mk_common_pkg(prepare_dir, ota_pkg)

    def mk_sys_pkg_inc(self, ota_process, old_pkg, diffpatch_dir,
                       prepare_dir, out_dir):
        sys_pkg_black_list = ['middleware', 'app']
        prepare_dir_inc = prepare_dir + "/all_in_one_inc/"
        old_dir = prepare_dir + "/all_in_one_old/"
        ota_pkg = out_dir + "/all_in_one_inc.zip"
        if os.path.exists(prepare_dir_inc):
            shutil.rmtree(prepare_dir_inc)
        os.makedirs(prepare_dir_inc)
        if not os.path.isfile(old_pkg):
            logging.error(f"package {old_pkg} not exits")
            sys.exit(-1)
        uncompress_zip(old_pkg, old_dir)
        self.add_file = ota_process
        self.old_dir = old_dir
        self.inc = True
        self.diffpatch_dir = diffpatch_dir
        for _, m in enumerate(self.medium):
            for key, part_conf in self.conf_d[m].items():
                if part_conf['base_name'] in sys_pkg_black_list:
                    self.conf_d[m][key]['ota_is_update'] = False
                else:
                    self.conf_d[m][key]['ota_update_mode'] = "image_diff"

        self.mk_common_pkg(prepare_dir_inc, ota_pkg)

    def mk_full_pkg(self, ota_process, app_dir, middleware_dir, app_param_dir,
                    param_update,  prepare_dir, out_dir):
        full_pkg_black_list = ['middleware', 'app', 'app_param']
        if not self.ota_sign:
            full_pkg_black_list += ['SBL', 'HSM_FW', 'keyimage']

        prepare_dir = prepare_dir + "/all_in_one_full"
        ota_pkg = out_dir + "/all_in_one_full.zip"
        if os.path.exists(prepare_dir):
            shutil.rmtree(prepare_dir)
        os.makedirs(prepare_dir)
        # self.handle_app_middleware_img(app_dir, "app")
        # self.handle_app_middleware_img(middleware_dir, "middleware")
        # if app_param_dir:
        #     self.mk_app_param_pkg(self, param_update,
        #                           app_param_dir, self.image_dir)
        for _, m in enumerate(self.medium):
            for key, part_conf in self.conf_d[m].items():
                if part_conf['part_type'] == 'PERMANENT':
                    continue
                if part_conf['base_name'] in full_pkg_black_list:
                    self.conf_d[m][key]['ota_is_update'] = False
                else:
                    self.conf_d[m][key]['ota_is_update'] = True

        self.add_file = ota_process
        self.mk_common_pkg(prepare_dir, ota_pkg)

    def mk_full_pkg_signed(self, ota_process, app_dir, middleware_dir,
                           app_param_dir,  param_update,
                           prepare_dir, out_dir):
        self.ota_sign = True
        full_pkg_black_list = ['middleware', 'app', 'app_param']
        if not self.ota_sign:
            full_pkg_black_list += ['SBL', 'HSM_FW', 'keyimage']

        prepare_dir = prepare_dir + "/all_in_one_full_signed"
        ota_pkg = out_dir + "/all_in_one_full_signed.zip"

        if os.path.exists(prepare_dir):
            shutil.rmtree(prepare_dir)
        os.makedirs(prepare_dir)
        # self.handle_app_middleware_img(app_dir, "app")
        # self.handle_app_middleware_img(middleware_dir, "middleware")
        # if app_param_dir:
        #     self.mk_app_param_pkg(self, param_update,
        #                           app_param_dir, self.image_dir)
        for _, m in enumerate(self.medium):
            for key, part_conf in self.conf_d[m].items():
                if part_conf['part_type'] == 'PERMANENT':
                    continue
                if part_conf['base_name'] in full_pkg_black_list:
                    self.conf_d[m][key]['ota_is_update'] = False
                else:
                    self.conf_d[m][key]['ota_is_update'] = True

        self.add_file = ota_process
        self.mk_common_pkg(prepare_dir, ota_pkg)

    def mk_full_pkg_inc(self, old_pkg, diffpatch_dir, ota_process, app_dir,
                        middleware_dir, app_param_dir, param_update,
                        prepare_dir, out_dir):
        full_pkg_black_list = ['middleware', 'app', 'app_param']
        if not self.ota_sign:
            full_pkg_black_list += ['SBL', 'HSM_FW', 'keyimage']

        prepare_dir_inc = prepare_dir + "/all_in_one_full_inc/"
        ota_pkg = out_dir + "/all_in_one_full_inc.zip"
        old_dir = prepare_dir + "/all_in_one_full_old/"
        if os.path.exists(prepare_dir_inc):
            shutil.rmtree(prepare_dir_inc)
        os.makedirs(prepare_dir_inc)
        if not os.path.isfile(old_pkg):
            logging.error(f"package {old_pkg} not exits")
            sys.exit(-1)
        uncompress_zip(old_pkg, old_dir)

        # self.handle_app_middleware_img(app_dir, "app")
        # self.handle_app_middleware_img(middleware_dir, "middleware")
        # if app_param_dir:
        #     self.mk_app_param_pkg(self, param_update,
        #                           app_param_dir, self.image_dir)
        for _, m in enumerate(self.medium):
            for key, part_conf in self.conf_d[m].items():
                if part_conf['part_type'] == 'PERMANENT':
                    continue
                if part_conf['base_name'] in full_pkg_black_list:
                    self.conf_d[m][key]['ota_is_update'] = False
                else:
                    self.conf_d[m][key]['ota_is_update'] = True
                    self.conf_d[m][key]['ota_update_mode'] = "image_diff"

        self.add_file = ota_process
        self.old_dir = old_dir
        self.inc = True
        self.diffpatch_dir = diffpatch_dir
        self.mk_common_pkg(prepare_dir_inc, ota_pkg)

    def mk_app_middleware_pkg(self, ota_process, _dir, part_name, prepare_dir,
                              out_dir):
        ota_pkg = out_dir + f"/{part_name}.zip"
        prepare_dir = prepare_dir + f"/{part_name}/"
        if os.path.exists(prepare_dir):
            shutil.rmtree(prepare_dir)
        os.makedirs(prepare_dir)

        if os.path.isfile(_dir) and _dir.endswith(".img"):
            img = _dir
        elif os.path.isdir(_dir):
            img = prepare_dir + f"{part_name}.img"
            self.mk_img(_dir, prepare_dir, part_name)
        else:
            logging.error("image or dir error")
            sys.exit(-1)

        self.image_dir = os.path.dirname(img)
        for _, m in enumerate(self.medium):
            for key, part_conf in self.conf_d[m].items():
                if part_conf['base_name'] != part_name:
                    self.conf_d[m][key]['ota_is_update'] = False
                else:
                    self.conf_d[m][key]['ota_is_update'] = True

        self.app_middleware = True
        self.add_file = ota_process
        self.mk_common_pkg(prepare_dir, ota_pkg)

    def mk_app_middleware_pkg_inc(self, old_pkg, ota_process, _dir, part_name,
                                  diffpatch_dir, prepare_dir, out_dir):
        old_dir = prepare_dir + f"/{part_name}_old"
        prepare_inc_dir = prepare_dir + f"/{part_name}_inc"
        ota_pkg = out_dir + f"/{part_name}_inc.zip"
        if os.path.exists(prepare_inc_dir):
            shutil.rmtree(prepare_inc_dir)
        os.makedirs(prepare_inc_dir)
        if not os.path.isfile(old_pkg):
            logging.error(f"package {old_pkg} not exits")
            sys.exit(-1)
        uncompress_zip(old_pkg, old_dir)

        if os.path.isfile(_dir) and _dir.endswith(".img"):
            img = _dir
        elif os.path.isdir(_dir):
            prepare_full_dir = prepare_dir + f"/{part_name}/"
            img = prepare_full_dir + f"{part_name}.img"
            self.mk_img(_dir, prepare_full_dir, part_name)
        else:
            logging.error("image or dir error")
            sys.exit(-1)
        self.image_dir = os.path.dirname(img)
        for _, m in enumerate(self.medium):
            for key, part_conf in self.conf_d[m].items():
                if part_conf['base_name'] != part_name:
                    self.conf_d[m][key]['ota_is_update'] = False
                else:
                    self.conf_d[m][key]['ota_is_update'] = True
                    self.conf_d[m][key]['ota_update_mode'] = "image_diff"

        self.old_dir = old_dir
        self.inc = True
        self.add_file = ota_process
        self.diffpatch_dir = diffpatch_dir
        self.mk_common_pkg(prepare_inc_dir, ota_pkg)

    def mk_app_param_pkg(self, ota_process, param_update, param_dir, out_dir):
        ota_pkg = out_dir + "/app_param.zip"
        compress_file_list = [param_update]
        if not os.path.isfile(param_update):
            logging.error("please add app_param update script")
            sys.exit(-1)
        if not os.path.isdir(param_dir):
            logging.error("app_param is not exists")
            sys.exit(-1)
        compress_file_list.append(param_dir)
        if not os.path.exists(out_dir):
            os.makedirs(out_dir)
        if os.path.isfile(ota_pkg):
            os.remove(ota_pkg)

        compress_file_list.append(ota_process)
        pkg_info = '\n'
        for c_file in compress_file_list:
            pkg_info += f"\t adding: {os.path.basename(c_file)}\n"
        logging.info(pkg_info)

        compress_zip(ota_pkg, compress_file_list)
        sigh_pkg(ota_pkg, self.sign_key)


class OTA_PKG_Tool(object):
    def __init__(self) -> None:
        pass

    def _add_common_args(self, sub_parser):
        sub_parser.add_argument('--sign_key',
                                help='Path to RSA private key file',
                                metavar='KEY',
                                required=False)
        sub_parser.add_argument('--prepare_dir',
                                help='Path to prepare package dir')
        sub_parser.add_argument('--out_dir',
                                help='Path to OTA package out dir')

    def run(self, argv):
        parser = argparse.ArgumentParser(description='HB OTA package tool')
        subparsers = parser.add_subparsers(title='subcommands')

        # unpack
        sub_parser = subparsers.add_parser('unpack',
                                           help='make OTA system package')
        sub_parser.add_argument('--ota_pkg',
                                help='Path to old OTA system package')
        sub_parser.add_argument('--out_dir',
                                help='Path to unpack dir')
        sub_parser.set_defaults(func=self.ota_unpack)

        # repack
        sub_parser = subparsers.add_parser('repack',
                                           help='make OTA system package')
        sub_parser.add_argument('--ota_process',
                                default="./ota_process",
                                help='Path to OTA process')
        sub_parser.add_argument('--partition_file',
                                help='Path to Partition table file')
        sub_parser.add_argument('--image_dir',
                                help='Path to all image dir')
        self._add_common_args(sub_parser)
        sub_parser.set_defaults(func=self.ota_repack)

        # system package
        sub_parser = subparsers.add_parser('sys_pkg',
                                           help='make OTA system package')
        sub_parser.add_argument('--ota_process',
                                help='Path to OTA process')
        sub_parser.add_argument('--partition_file',
                                help='Path to Partition table file')
        sub_parser.add_argument('--image_dir',
                                help='Path to all image dir')
        self._add_common_args(sub_parser)
        sub_parser.set_defaults(func=self.mk_sys_pkg)

        # system difference package
        sub_parser = subparsers.add_parser('sys_pkg_inc',
                                           help="make OTA system difference\
package")
        sub_parser.add_argument('--ota_process',
                                help='Path to OTA process')
        sub_parser.add_argument('--partition_file',
                                help='Path to Partition table file')
        sub_parser.add_argument('--image_dir',
                                help='Path to all image dir')
        sub_parser.add_argument('--old_pkg',
                                help='Path to old OTA system package')
        sub_parser.add_argument('--diffpatch_dir',
                                help='Path to old OTA hdiffpatch tools dir')
        self._add_common_args(sub_parser)
        sub_parser.set_defaults(func=self.mk_sys_pkg_inc)

        # full package
        sub_parser = subparsers.add_parser('full_pkg',
                                           help='make OTA full package')
        sub_parser.add_argument('--ota_process',
                                help='Path to OTA process')
        sub_parser.add_argument('--partition_file',
                                help='Path to Partition table file')
        sub_parser.add_argument('--image_dir',
                                help='Path to all image dir')
        sub_parser.add_argument('--app_dir',
                                default=None,
                                help='Path to app dir')
        sub_parser.add_argument('--middleware_dir',
                                default=None,
                                help='Path to middleware dir')
        sub_parser.add_argument('--app_param',
                                default=None,
                                help='Path to app param dir')
        sub_parser.add_argument('--param_update',
                                default=None,
                                help='Path to app param updating script')
        self._add_common_args(sub_parser)
        sub_parser.set_defaults(func=self.mk_full_pkg)

        # full package signed
        sub_parser = subparsers.add_parser('full_pkg_signed',
                                           help='make OTA full package')
        sub_parser.add_argument('--ota_process',
                                help='Path to OTA process')
        sub_parser.add_argument('--partition_file',
                                help='Path to Partition table file')
        sub_parser.add_argument('--image_dir',
                                help='Path to all image dir')
        sub_parser.add_argument('--app_dir',
                                default=None,
                                help='Path to app dir')
        sub_parser.add_argument('--middleware_dir',
                                default=None,
                                help='Path to middleware dir')
        sub_parser.add_argument('--app_param',
                                default=None,
                                help='Path to app param dir')
        sub_parser.add_argument('--param_update',
                                default=None,
                                help='Path to app param updating script')
        self._add_common_args(sub_parser)
        sub_parser.set_defaults(func=self.mk_full_pkg_signed)

        # full difference package
        sub_parser = subparsers.add_parser('full_pkg_inc',
                                           help="make OTA full difference\
package")
        sub_parser.add_argument('--ota_process',
                                help='Path to OTA process')
        sub_parser.add_argument('--partition_file',
                                help='Path to Partition table file')
        sub_parser.add_argument('--image_dir',
                                help='Path to all image dir')
        sub_parser.add_argument('--old_pkg',
                                help='Path to old OTA full package')
        sub_parser.add_argument('--app_dir',
                                help='Path to app dir')
        sub_parser.add_argument('--middleware_dir',
                                help='Path to middleware dir')
        sub_parser.add_argument('--app_param',
                                help='Path to app param dir')
        sub_parser.add_argument('--param_update',
                                help='Path to app param updating script')
        sub_parser.add_argument('--diffpatch_dir',
                                help='Path to old OTA hdiffpatch tools dir')
        self._add_common_args(sub_parser)
        sub_parser.set_defaults(func=self.mk_full_pkg_inc)

        # middleware package
        sub_parser = subparsers.add_parser('middleware_pkg',
                                           help='make OTA middleware package')
        sub_parser.add_argument('--ota_process',
                                help='Path to OTA process')
        sub_parser.add_argument('--partition_file',
                                help='Path to Partition table file')
        sub_parser.add_argument('--middleware_dir',
                                help='Path to middleware dir')
        self._add_common_args(sub_parser)
        sub_parser.set_defaults(func=self.mk_middleware_pkg)

        # middleware difference package
        sub_parser = subparsers.add_parser('middleware_pkg_inc',
                                           help='make OTA middleware\
difference package')
        sub_parser.add_argument('--ota_process',
                                help='Path to OTA process')
        sub_parser.add_argument('--partition_file',
                                help='Path to Partition table file')
        sub_parser.add_argument('--middleware_dir',
                                help='Path to middleware dir')
        sub_parser.add_argument('--old_pkg',
                                help='Path to old OTA middleware package')
        sub_parser.add_argument('--diffpatch_dir',
                                help='Path to old OTA hdiffpatch tools dir')
        self._add_common_args(sub_parser)
        sub_parser.set_defaults(func=self.mk_middleware_pkg_inc)

        # app package
        sub_parser = subparsers.add_parser('app_pkg',
                                           help='make OTA app package')
        sub_parser.add_argument('--ota_process',
                                help='Path to OTA process')
        sub_parser.add_argument('--partition_file',
                                help='Path to Partition table file')
        sub_parser.add_argument('--app_dir',
                                help='Path to app dir')
        self._add_common_args(sub_parser)
        sub_parser.set_defaults(func=self.mk_app_pkg)

        # app difference package
        sub_parser = subparsers.add_parser('app_pkg_inc',
                                           help='make OTA app difference\
package')
        sub_parser.add_argument('--ota_process',
                                help='Path to OTA process')
        sub_parser.add_argument('--partition_file',
                                help='Path to Partition table file')
        sub_parser.add_argument('--app_dir',
                                help='Path to app dir')
        sub_parser.add_argument('--old_pkg',
                                help='Path to old OTA app package')
        sub_parser.add_argument('--diffpatch_dir',
                                help='Path to old OTA hdiffpatch tools dir')
        self._add_common_args(sub_parser)
        sub_parser.set_defaults(func=self.mk_app_pkg_inc)

        # app param package
        sub_parser = subparsers.add_parser('app_param_pkg',
                                           help='make OTA app param package')
        sub_parser.add_argument('--ota_process',
                                help='Path to OTA process')
        sub_parser.add_argument('--app_param',
                                help='Path to app param dir')
        sub_parser.add_argument('--param_update',
                                help='Path to app param updating script')
        sub_parser.add_argument('--sign_key',
                                help='Path to RSA private key file',
                                metavar='KEY',
                                required=False)
        sub_parser.add_argument('--out_dir',
                                help='Path to OTA package out dir')
        sub_parser.set_defaults(func=self.mk_app_param_pkg)

        # Parameter parsing
        args = parser.parse_args(argv[1:])
        try:
            if argv[1] == "unpack":
                self.ota_pkg = HBOTAPKG()
            else:
                self.ota_pkg = HBOTAPKG(args.sign_key)
            args.func(args)
        except HBOTAError as e:
            sys.stderr.write('{}: {}\n'.format(argv[0], e.message))
            sys.exit(1)

    def ota_unpack(self, args):
        logging.info("unpack ota package")
        self.ota_pkg.ota_unpack(args.ota_pkg, args.out_dir)

    def ota_repack(self, args):
        logging.info("generate package all_in_one_repack")
        self.ota_pkg.image_dir = args.image_dir
        self.ota_pkg.part_file_parse(args.partition_file)
        self.ota_pkg.mk_repack(args.ota_process, args.prepare_dir,
                               args.out_dir)

    def mk_sys_pkg(self, args):
        logging.info("generate package all_in_one")
        self.ota_pkg.image_dir = args.image_dir
        self.ota_pkg.part_file_parse(args.partition_file)
        self.ota_pkg.mk_sys_pkg(args.ota_process, args.prepare_dir,
                                args.out_dir)
        logging.info("package all_in_one generation completed")

    def mk_sys_pkg_inc(self, args):
        logging.info("generate package all_in_one_inc")
        self.ota_pkg.image_dir = args.image_dir
        self.ota_pkg.part_file_parse(args.partition_file)
        self.ota_pkg.mk_sys_pkg_inc(args.ota_process, args.old_pkg,
                                    args.diffpatch_dir, args.prepare_dir,
                                    args.out_dir)
        logging.info("package all_in_one_inc generation completed")

    def mk_full_pkg(self, args):
        logging.info("generate package all_in_one_full")
        self.ota_pkg.image_dir = args.image_dir
        self.ota_pkg.part_file_parse(args.partition_file)
        self.ota_pkg.mk_full_pkg(args.ota_process, args.app_dir,
                                 args.middleware_dir, args.app_param,
                                 args.param_update, args.prepare_dir,
                                 args.out_dir)
        logging.info("package all_in_one_full generation completed")

    def mk_full_pkg_signed(self, args):
        logging.info("generate package all_in_one_full")
        self.ota_pkg.image_dir = args.image_dir
        self.ota_pkg.part_file_parse(args.partition_file)
        self.ota_pkg.mk_full_pkg_signed(args.ota_process, args.app_dir,
                                        args.middleware_dir, args.app_param,
                                        args.param_update, args.prepare_dir,
                                        args.out_dir)
        logging.info("package mk_full_pkg_signed generation completed")

    def mk_full_pkg_inc(self, args):
        logging.info("generate package all_in_one_full_inc")
        self.ota_pkg.image_dir = args.image_dir
        self.ota_pkg.part_file_parse(args.partition_file)
        self.ota_pkg.mk_full_pkg_inc(args.old_pkg, args.diffpatch_dir,
                                     args.ota_process, args.app_dir,
                                     args.middleware_dir, args.app_param,
                                     args.param_update, args.prepare_dir,
                                     args.out_dir)
        logging.info("package all_in_one_full_inc generation completed")

    def mk_middleware_pkg(self, args):
        logging.info("generate package middleware")
        self.ota_pkg.part_file_parse(args.partition_file)
        self.ota_pkg.mk_app_middleware_pkg(args.ota_process,
                                           args.middleware_dir, "middleware",
                                           args.prepare_dir, args.out_dir)
        logging.info("package middleware generation completed")

    def mk_middleware_pkg_inc(self, args):
        logging.info("generate package middleware_inc")
        self.ota_pkg.part_file_parse(args.partition_file)
        self.ota_pkg.mk_app_middleware_pkg_inc(args.old_pkg, args.ota_process,
                                               args.middleware_dir,
                                               "middleware",
                                               args.diffpatch_dir,
                                               args.prepare_dir,
                                               args.out_dir)
        logging.info("package middleware_inc generation completed")

    def mk_app_pkg(self, args):
        logging.info("generate package app")
        self.ota_pkg.part_file_parse(args.partition_file)
        self.ota_pkg.mk_app_middleware_pkg(args.ota_process, args.app_dir,
                                           "app", args.prepare_dir,
                                           args.out_dir)
        logging.info("package app generation completed")

    def mk_app_pkg_inc(self, args):
        logging.info("generate package app_inc")
        self.ota_pkg.part_file_parse(args.partition_file)
        self.ota_pkg.mk_app_middleware_pkg_inc(args.old_pkg, args.ota_process,
                                               args.app_dir, "app",
                                               args.diffpatch_dir,
                                               args.prepare_dir, args.out_dir)
        logging.info("package app_inc generation completed")

    def mk_app_param_pkg(self, args):
        logging.info("generate package app_param")
        self.ota_pkg.mk_app_param_pkg(args.ota_process, args.param_update,
                                      args.app_param, args.out_dir)
        logging.info("package app_param generation completed")


if __name__ == '__main__':
    logging.basicConfig(level=logging.INFO,
                        # filename=os.getenv('OUTPUT_LOG_DIR') +
                        # "/package.log",
                        # filemode="a",
                        format="%(asctime)s - %(filename)s - %(funcName)s \
- %(levelname)s - %(message)s")
    tool = OTA_PKG_Tool()
    tool.run(sys.argv)
