#!/usr/bin/env python

import sys
from serial import Serial


header_prefix = '<<<-'
footer_prefix = '->>>'
wagman_device = '/dev/waggle_sysmon'

def wagman_client(args):
    serial = Serial(wagman_device, 115200)

    command = ' '.join(args)
    serial.write(command.encode('ascii'))
    serial.write(b'\n')

    # header
    header = serial.readline().decode().strip()

    if header.startswith(header_prefix):
        # TODO parse header line
        pass
    else:
        serial.close()
        raise Exception('header not found')


    while (1):
        line = serial.readline().decode().strip()
    
        if line.startswith(footer_prefix):
            break
        
        yield line
    
    serial.close()   
    


if __name__ == "__main__":

    for line in wagman_client(sys.argv[1:]):
        print line





