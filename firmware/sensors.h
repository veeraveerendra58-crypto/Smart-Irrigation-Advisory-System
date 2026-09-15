#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>



void initSensors();
void sampleSensors();
int averageAnalogRead(int pin, int samples = 10);
float rawToMoisturePercent(int raw);
float getAvgTemp();
float getAvgHumidity();
float getAvgLight();
float getAvgSoilTemp();


#endif
