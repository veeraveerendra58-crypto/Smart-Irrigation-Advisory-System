#ifndef MOISTURE_H
#define MOISTURE_H


void initMoisture();
void loadMoisturePreferences();

//void startMoistureCalibration();
//void startFieldCapacityCalibration();

float readMoisturePoint();
float computeAverageMoisture(float points[], int count);
float convertToWaterMM(float moisturePercent);

#endif
