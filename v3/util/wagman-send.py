from serial import Serial

serial = Serial('/dev/waggle_sysmon', 115200)

while True:
    serial.write(input('$ ').encode('ascii'))
    serial.write(b'\n')

serial.close()
