#ifndef DHT22_H
#define DHT22_H

#include <Arduino.h>

class DHT22 {
public:
  DHT22(uint8_t pin);
  void begin(uint8_t usec = 55);

  float readTemperature();   // Celsius only
  float readHumidity();

private:
  uint8_t _pin;
  uint8_t data[5];
  uint32_t _maxcycles;
  unsigned long _lastreadtime;
  bool _lastresult;
  uint8_t pullTime;

  bool read(bool force = false);
  uint32_t expectPulse(bool level);
};

#endif