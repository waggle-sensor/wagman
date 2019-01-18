#ifndef __H_EEPROM__
#define __H_EEPROM__

#include <Wire.h>

class EEPROMInterface {
public:

    virtual byte read(int addr) = 0;
    virtual void write(int addr, byte value) = 0;

    template <class T>
    void get(int addr, T &obj) {
        byte *objbytes = (byte *)&obj;

        for (int i = 0; i < sizeof(obj); i++) {
            objbytes[i] = read(addr + i);
        }
    }

    template <class T>
    void put(int addr, T obj) {
        byte *objbytes = (byte *)&obj;

        for (int i = 0; i < sizeof(obj); i++) {
            write(addr + i, objbytes[i]);
        }
    }
};

template <int N>
class MockEEPROM : public EEPROMInterface {
public:

    void clear() {
        for (int i = 0; i < N; i++) {
            data[i] = 0;
        }
    }

    int length() const {
        return N;
    }

    byte read(int addr) {
        return data[addr];
    }

    void write(int addr, byte value) {
        data[addr] = value;
    }

private:

    byte data[N];
};

class ExternalEEPROM : public EEPROMInterface {
public:

    bool waitForDevice() const {
      while (true) {
          Wire.beginTransmission(busaddr);

          if (Wire.endTransmission() == 0) {
              return true;
          }
      }

      return false;
    }

    void writeAddress(int addr) {
      Wire.write((byte)((addr >> 8) & 0xff));
      Wire.write((byte)((addr >> 0) & 0xff));
    }

    byte read(int addr) {
        waitForDevice();

        Wire.beginTransmission(busaddr);
        writeAddress(addr);
        Wire.endTransmission();

        Wire.requestFrom(busaddr, 1);

        while (Wire.available() == 0) {
            // TODO ensure timeout
        }

        return Wire.read();
    }

    void write(int addr, byte data) {
        waitForDevice();

        Wire.beginTransmission(busaddr);
        writeAddress(addr);
        Wire.write(data);
        Wire.endTransmission(); // TODO Add Error function for error handling.
    }

private:

    static const int busaddr = 0x50;
};

extern ExternalEEPROM EEPROM;
// extern MockEEPROM<4096> EEPROM;

#endif
