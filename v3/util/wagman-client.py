#!/usr/bin/env python

import sys
from serial import Serial


header_prefix = '<<<-'
footer_prefix = '->>>'

serial = Serial('/dev/waggle_sysmon', 115200)

command = ' '.join(sys.argv[1:])
serial.write(command.encode('ascii'))
serial.write(b'\n')

# header
header = serial.readline().decode().strip()

if header.startswith(header_prefix):
    print "header found"
else:
    print "header not found"
    serial.close()
    sys.exit(1)


while (1):
    line = serial.readline().decode().strip()
    
    if line.startswith(footer_prefix):
        break
        
    print line
    
serial.close()   
    






