#!/usr/bin/env python3
import binascii
import uuid
from struct import pack, unpack

OS_TYPES = {
    0x00: 'Empty',
    0xEE: 'GPT Protective',
    0xEF: 'UEFI System Partition'
}

PARTITION_TYPE_GUIDS = {
    '024DEE41-33E7-11D3-9D69-0008C781F39F': 'Legacy MBR',
    'C12A7328-F81F-11D2-BA4B-00A0C93EC93B': 'EFI System Partition',
    '21686148-6449-6E6F-744E-656564454649': 'BIOS boot partition',
    '0FC63DAF-8483-4772-8E79-3D69D8477DE4': 'Linux filesystem data',
    '4F68BCE3-E8CD-4DB1-96E7-FBCAF984B709': 'Root partition (x86-64)',
    '0657FD6D-A4AB-43C4-84E5-0933C84B4F4F': 'Swap partition',
    '7C3457EF-0000-11AA-AA11-00306543ECAC': 'Apple APFS'
}


def convert_uuid(input_uuid):
    stripped_uuid = input_uuid.replace("-", "")

    part1 = stripped_uuid[0:8]
    part2 = stripped_uuid[8:12]
    part3 = stripped_uuid[12:16]
    part4 = stripped_uuid[16:20]
    part5 = stripped_uuid[20:]

    output_uuid = f"{part1[6:8]}{part1[4:6]}{part1[2:4]}{part1[0:2]}-\
        {part2[2:4]}{part2[0:2]}-{part3[2:4]}{part3[0:2]}-{part4}-{part5}"

    return output_uuid.replace("    ", "")


def decode_gpt_partition_type_guid(guid):
    if isinstance(guid, uuid.UUID):
        guid = str(guid)

    guid = guid.upper()
    return PARTITION_TYPE_GUIDS.get(guid, '?')


def decode_gpt_partition_entry_attributes(attribute_value):
    r = []
    if (attribute_value & 0x1):
        r.append('Required Partition')
    if (attribute_value & 0x2):
        r.append('No Block IO Protocol')
    if (attribute_value & 0x4):
        r.append('Legacy BIOS Bootable')
    return r


# guid bytes to guid object
def decode_guid(guid_as_bytes):
    return uuid.UUID(bytes_le=guid_as_bytes)


# this is a funny field
# it is utf-16 encoded
# and padded with zeros, so like a null terminated string
def nts_to_str(buf):
    s = buf.decode('UTF-16')
    return s.split('\0', 1)[0]


class GPTHeader():
    def __init__(self,
                 signature,
                 revision,
                 header_size,
                 header_crc32,
                 reserved,
                 my_lba,
                 alternate_lba,
                 first_usable_lba,
                 last_usable_lba,
                 disk_guid,
                 partition_entry_lba,
                 number_of_partition_entries,
                 size_of_partition_entry,
                 partition_entry_array_crc32):
        self.signature = signature
        self.revision = revision
        self.header_size = header_size
        self.header_crc32 = header_crc32
        self.reserved = reserved
        self.my_lba = my_lba
        self.alternate_lba = alternate_lba
        self.first_usable_lba = first_usable_lba
        self.last_usable_lba = last_usable_lba
        self.disk_guid = disk_guid
        self.partition_entry_lba = partition_entry_lba
        self.number_of_partition_entries = number_of_partition_entries
        self.size_of_partition_entry = size_of_partition_entry
        self.partition_entry_array_crc32 = partition_entry_array_crc32

    def is_valid(self):
        return self.signature == 'EFI PART'.encode('ascii')

    def calculate_header_crc32(self):
        header_crc32_input = pack('<8s 4s I I 4s Q Q Q Q 16s Q I I I',
                                  self.signature,
                                  self.revision,
                                  self.header_size,
                                  0,  # set to 0 for crc32 calculation
                                  self.reserved,
                                  self.my_lba,
                                  self.alternate_lba,
                                  self.first_usable_lba,
                                  self.last_usable_lba,
                                  self.disk_guid,
                                  self.partition_entry_lba,
                                  self.number_of_partition_entries,
                                  self.size_of_partition_entry,
                                  self.partition_entry_array_crc32)
        return binascii.crc32(header_crc32_input) & 0xffffffff


class GPTPartitionEntry():
    def __init__(
            self,
            partition_type_guid,
            unique_partition_guid,
            starting_lba,
            ending_lba,
            attributes,
            partition_name):
        if isinstance(partition_type_guid, str):
            c_partition_type_guid = convert_uuid(partition_type_guid)
            self.partition_type_guid_raw = uuid.UUID(
                c_partition_type_guid).bytes
            self.partition_type_guid = partition_type_guid
        else:
            self.partition_type_guid_raw = partition_type_guid
            self.partition_type_guid = decode_guid(partition_type_guid)

        self.partition_type = decode_gpt_partition_type_guid(
            self.partition_type_guid)
        self.unique_partition_guid_raw = unique_partition_guid
        self.unique_partition_guid = decode_guid(unique_partition_guid)
        self.starting_lba = starting_lba
        self.ending_lba = ending_lba
        self.attributes_raw = attributes
        self.attributes = decode_gpt_partition_entry_attributes(attributes)
        self.partition_name_raw = partition_name
        self.partition_name = nts_to_str(partition_name)

    def is_empty(self):
        return all(x == 0 for x in self.partition_type_guid_raw)


def calculate_partition_entry_array_crc32(data):
    return binascii.crc32(data) & 0xffffffff


def encode_gpt_header(gpt_header):
    data = pack(
        '< 8s 4s I I 4s Q Q Q Q 16s Q I I I',
        gpt_header.signature,
        gpt_header.revision,
        gpt_header.header_size,
        gpt_header.header_crc32,
        gpt_header.reserved,
        gpt_header.my_lba,
        gpt_header.alternate_lba,
        gpt_header.first_usable_lba,
        gpt_header.last_usable_lba,
        gpt_header.disk_guid,
        gpt_header.partition_entry_lba,
        gpt_header.number_of_partition_entries,
        gpt_header.size_of_partition_entry,
        gpt_header.partition_entry_array_crc32)
    return data


def decode_gpt_header(data):
    (signature,
     revision,
     header_size,
     header_crc32,
     reserved,
     my_lba,
     alternate_lba,
     first_usable_lba,
     last_usable_lba,
     disk_guid,
     partition_entry_lba,
     number_of_partition_entries,
     size_of_partition_entry,
     partition_entry_array_crc32) = unpack(
         '< 8s 4s I I 4s Q Q Q Q 16s Q I I I',
         data[0:92])
    gpt_header = GPTHeader(signature,
                           revision,
                           header_size,
                           header_crc32,
                           reserved,
                           my_lba,
                           alternate_lba,
                           first_usable_lba,
                           last_usable_lba,
                           disk_guid,
                           partition_entry_lba,
                           number_of_partition_entries,
                           size_of_partition_entry,
                           partition_entry_array_crc32)
    return gpt_header


def encode_gpt_partition_entry(gpt_partition_entry):
    data = pack('<16s 16s Q Q Q 72s',
                gpt_partition_entry.partition_type_guid_raw,
                gpt_partition_entry.unique_partition_guid_raw,
                gpt_partition_entry.starting_lba,
                gpt_partition_entry.ending_lba,
                gpt_partition_entry.attributes_raw,
                gpt_partition_entry.partition_name_raw)
    return data


def decode_gpt_partition_entry(data):
    (partition_type_guid,
     unique_partition_guid,
     starting_lba,
     ending_lba,
     attributes,
     partition_name) = unpack('< 16s 16s Q Q Q 72s', data[0:128])
    return GPTPartitionEntry(
        partition_type_guid,
        unique_partition_guid,
        starting_lba,
        ending_lba,
        attributes,
        partition_name)


def encode_gpt_partition_entry_array(gpt_partition_entries, size, count):
    data = bytearray()
    for i in range(0, count):
        d = encode_gpt_partition_entry(gpt_partition_entries[i])
        data.extend(d)
        # fill with zeroes if less than size
        if len(d) < size:
            data.extend(bytearray(size - len(d)))
    return bytes(data)


def decode_gpt_partition_entry_array(data, size, count):
    entries = []
    for i in range(0, count):
        offset = i * size
        gpt_partition_entry = decode_gpt_partition_entry(
            data[offset:offset+size])
        entries.append(gpt_partition_entry)
    return entries
