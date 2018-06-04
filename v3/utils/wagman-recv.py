from serial import Serial

serial = Serial('/dev/waggle_sysmon', 115200)

while True:
    print(serial.readline().decode().strip())

serial.close()
