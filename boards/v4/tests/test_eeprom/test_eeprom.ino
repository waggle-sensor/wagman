#include <Wire.h>

#define disk1 0x50    //Address of 24LC256 eeprom chip

const int pageSize = 64;

void setup(void)
{
    SerialUSB.begin(115200);
    Wire.begin();
}

void loop() {
    delay(1000);
}

void writeAddress(int addr) {
    Wire.write((addr >> 8) & 0xff);
    Wire.write(addr & 0xff);
}

void EEPROMWriteByte(int port, int addr, int data) {
    Wire.beginTransmission(port);
    Wire.write(0xa0);
    writeAddress(addr)
    Wire.write(data);
    Wire.endTransmission();
}

void EEPROMWritePage(int port, int pageNum, int *pageData) {
    int addr = pageSize * pageNum;

    Wire.beginTransmission(port);
    Wire.write(0xa0);
    writeAddress(addr);

    for (int i = 0; i < pageSize; i++) {
        Wire.write(pageData[i]);
    }

    Wire.endTransmission();
}

int EEPROMReadByte(int port, unsigned long timeout) {
    Wire.beginTransmission(port);
    Wire.write(0xa1);
    Wire.endTransmission();

    unsigned long startTime = millis();

    Wire.requestFrom(port, 1);

    while ((millis() - startTime) < timeout) {
        if (Wire.available()) {
            return Wire.read();
        }
    }

    return -1;
}

int EEPROMReadByteAtAddress(int port, int addr, unsigned long timeout) {
    Wire.beginTransmission(port);
    Wire.write(0xa0);
    writeAddress(addr)
    Wire.endTransmission();

    return EEPROMReadByte(port, timeout);
}

int EEPROMRead(int port, int addr, byte *data, int count, unsigned long timeout) {
    Wire.beginTransmission(port);
    Wire.write(0xa0);
    writeAddress(addr)
    Wire.endTransmission();

    Wire.requestFrom(port, count);

    unsigned long startTime = millis();
    int n = 0;

    while ((millis() - startTime < timeout) && (n < count)) {
        if (Wire.available()) {
            data[n++] = Wire.read();
        }
    }

    return n;
}
