# This file is part of the Waggle Platform.  Please see the file
# LICENSE.waggle.txt for the legal details of the copyright and software
# license.  For more details on the Waggle project, visit:
#          http://www.wa8.gl
from serial import Serial


ser = Serial('/dev/tty.usbmodem1421', 115200)

ser.readline()  # skip possibly incomplete line
mac = ser.readline().decode().strip()
board_id = input('{} ID: '.format(mac)).strip()

with open('boards.csv', 'a') as outfile:
    outfile.write('{},{}\n'.format(board_id, mac))
