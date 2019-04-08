# This file is part of the Waggle Platform.  Please see the file
# LICENSE.waggle.txt for the legal details of the copyright and software
# license.  For more details on the Waggle project, visit:
#          http://www.wa8.gl
def format_mac(mac):
    return ''.join(map(format_block, mac.split(':')[2:]))


def format_block(x):
    return '{:02X}'.format(int(x, 16))


def cleanup(line):
    return line.strip()


def relevant(line):
    return not line.startswith('#')


for line in filter(relevant, map(cleanup, open('boards.csv'))):
    ident, mac = line.split(',')
    print(','.join([ident, format_mac(mac)]))
