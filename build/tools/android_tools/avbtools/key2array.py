#!/usr/bin/python3

import sys
import os

DEBUG = False


def printline():
    print('-'*80)


def ShowCmdArgs():
    i = 0
    for arg in sys.argv:
        print('arg', i, ':', arg)
        i = i + 1


def usage():
    printline()
    print('Usage:')
    print('key2array.py -i keyfilename(private or public) -o cfilename')
    printline()


def readbin(filename=''):
    if filename == '':
        return ''
    else:
        try:
            fd = open(filename, 'rb')
        except IOError:
            print('Open ' + filename + ' Fail!')
            return ''
        else:
            buff = fd.read()
            fd.close()
            return buff


def main():
    iname = ''
    oname = ''
    if DEBUG:
        ShowCmdArgs()
    if len(sys.argv) == 3:
        if sys.argv[1] == '-i':
            iname = sys.argv[2]
        else:
            usage()
            return
    elif len(sys.argv) == 5:
        if sys.argv[1] == '-i':
            iname = sys.argv[2]
            if sys.argv[3] == '-o':
                oname = sys.argv[4]
            else:
                usage()
                return
        elif sys.argv[1] == '-o':
            oname = sys.argv[2]
            if sys.argv[3] == '-i':
                iname = sys.argv[4]
            else:
                usage()
                return
        else:
            usage()
            return
    else:
        usage()
        return

    cmd = './avbtool extract_public_key --key ' + \
        iname + ' --output ' + iname + '-bin'
    print(cmd)
    os.system(cmd)
    iname = iname + '-bin'
    if iname != '':
        print('In: ' + iname)

    if oname != '':
        print('Out: ' + oname)

    ibuff = readbin(iname).hex()

    keystring = ''
    for i in range(0, len(ibuff), 2):
        if i % 16 == 0:
            keystring += '\t'
        keystring += f"0x{ibuff[i:i+2]}, "
        if (i // 2 + 1) % 8 == 0:
            keystring += '\n'

    if oname == '':
        print(keystring)
    else:
        try:
            fd = open(oname, 'w')
            fd.write('static unsigned char avb_root_pub[520] = {\n')
        except IOError:
            print('Open ' + oname + ' Fail')
        else:
            fd.write(keystring)
            fd.write('};')
#            print keystring
            fd.close()

    print('Tatol len: ' + str(len(ibuff) // 2))


if __name__ == '__main__':
    main()
