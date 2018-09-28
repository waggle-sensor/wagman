#ifndef __H_EEPROM__
#define __H_EEPROM__

typedef unsigned char byte;

template <int N>
class MockEEPROM {
public:

    void clear() {
        for (int i = 0; i < N; i++) {
            data[i] = 0;
        }
    }

    int length() const {
        return N;
    }

    byte read(int addr) const {
        return data[addr];
    }

    void write(int addr, byte value) {
        data[addr] = value;
    }

    template <class T>
    void get(int addr, T &value) const {
        T *ptr = (T *)(&data[addr]);
        value = *ptr;
    }

    template <class T>
    void put(int addr, T value) {
        T *ptr = (T *)(&data[addr]);
        *ptr = value;
    }

private:

    byte data[N];
};

MockEEPROM<4096> EEPROM;

#endif
