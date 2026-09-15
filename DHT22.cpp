#include "DHT22.h"

#define MIN_INTERVAL 2000
#define TIMEOUT UINT32_MAX

// Interrupt lock (from original library)
class InterruptLock {
public:
  InterruptLock() { noInterrupts(); }
  ~InterruptLock() { interrupts(); }
};

DHT22::DHT22(uint8_t pin) {
  _pin = pin;
  _maxcycles = microsecondsToClockCycles(1000);
  _lastreadtime = 0;
  _lastresult = false;
}

void DHT22::begin(uint8_t usec) {
  pinMode(_pin, INPUT_PULLUP);
  _lastreadtime = millis() - MIN_INTERVAL;
  pullTime = usec;
}

float DHT22::readTemperature() {
  if (!read()) return NAN;

  float t = ((word)(data[2] & 0x7F) << 8) | data[3];
  t *= 0.1;
  if (data[2] & 0x80) t *= -1;
  return t;
}

float DHT22::readHumidity() {
  if (!read()) return NAN;

  float h = ((word)data[0] << 8) | data[1];
  return h * 0.1;
}

bool DHT22::read(bool force) {

  uint32_t currenttime = millis();
  if (!force && ((currenttime - _lastreadtime) < MIN_INTERVAL))
    return _lastresult;

  _lastreadtime = currenttime;

  for (int i = 0; i < 5; i++) data[i] = 0;

#if defined(ESP8266)
  yield();
#endif

  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);
  delayMicroseconds(1100);   // DHT22 start pulse
  pinMode(_pin, INPUT_PULLUP);
  delayMicroseconds(pullTime);

  uint32_t cycles[80];

  {
    InterruptLock lock;

    if (expectPulse(LOW) == TIMEOUT) return _lastresult = false;
    if (expectPulse(HIGH) == TIMEOUT) return _lastresult = false;

    for (int i = 0; i < 80; i += 2) {
      cycles[i] = expectPulse(LOW);
      cycles[i + 1] = expectPulse(HIGH);
    }
  }

  for (int i = 0; i < 40; ++i) {
    uint32_t lowCycles = cycles[2 * i];
    uint32_t highCycles = cycles[2 * i + 1];

    if ((lowCycles == TIMEOUT) || (highCycles == TIMEOUT))
      return _lastresult = false;

    data[i / 8] <<= 1;
    if (highCycles > lowCycles)
      data[i / 8] |= 1;
  }

  if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))
    return _lastresult = true;
  else
    return _lastresult = false;
}

uint32_t DHT22::expectPulse(bool level) {

#if (F_CPU > 16000000L) || (F_CPU == 0L)
  uint32_t count = 0;
#else
  uint16_t count = 0;
#endif

  while (digitalRead(_pin) == level) {
    if (count++ >= _maxcycles)
      return TIMEOUT;
  }

  return count;
}
