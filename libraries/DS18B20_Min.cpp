#include "DS18B20_Min.h"

#define STARTCONVO  0x44
#define READSCRATCH 0xBE

DS18B20_Min::DS18B20_Min(uint8_t pin) : _wire(pin) {
    deviceFound = false;
}

void DS18B20_Min::begin() {
    _wire.reset_search();
    if (_wire.search(address)) {
        if (OneWire::crc8(address, 7) == address[7]) {
            deviceFound = true;
        }
    }
}

void DS18B20_Min::requestTemperature() {
    if (!deviceFound) return;

    _wire.reset();
    _wire.select(address);
    _wire.write(STARTCONVO);
}

float DS18B20_Min::getTemperatureC() {
    if (!deviceFound) return NAN;

    uint8_t data[9];

    _wire.reset();
    _wire.select(address);
    _wire.write(READSCRATCH);

    for (int i = 0; i < 9; i++)
        data[i] = _wire.read();

    if (OneWire::crc8(data, 8) != data[8])
        return NAN;

    int16_t raw = (data[1] << 8) | data[0];
    return (float)raw / 16.0;  // 12-bit default resolution
}
