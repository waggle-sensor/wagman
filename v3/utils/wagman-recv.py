# This file is part of the Waggle Platform.  Please see the file
# LICENSE.waggle.txt for the legal details of the copyright and software
# license.  For more details on the Waggle project, visit:
#          http://www.wa8.gl
from serial import Serial

serial = Serial('/dev/waggle_sysmon', 115200)

while True:
    print(serial.readline().decode().strip())

serial.close()
