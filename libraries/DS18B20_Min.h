#ifndef DS18B20_MIN_H
#define DS18B20_MIN_H

#include <Arduino.h>
#include <OneWire.h>

class DS18B20_Min {
public:
    DS18B20_Min(uint8_t pin);
    void begin();
    void requestTemperature();
    float getTemperatureC();

private:
    OneWire _wire;
    uint8_t address[8];
    bool deviceFound;
};

#endif