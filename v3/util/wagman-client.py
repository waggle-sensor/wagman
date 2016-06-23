#!/usr/bin/env python

import sys
from serial import Serial

serial = Serial('/dev/waggle_sysmon', 115200)

command = ' '.join(sys.argv)
serial.write(command.encode('ascii'))
serial.write(b'\n')

# header
header = serial.readline().decode().strip()

if header.startswith('<<<-'):
    print "header found"
else:
    print "header not found"
    serial.close()
    sys.exit(1)


while (1):
    line = serial.readline().decode().strip()
    
    if line.startswith('<<<-'):
        break
        
    print line
    
serial.close()   
    






