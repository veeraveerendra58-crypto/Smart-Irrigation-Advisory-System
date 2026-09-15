#ifndef CALIBRATION_H
#define CALIBRATION_H

void startMoistureCalibration_dry();
void startMoistureCalibration_wet();
void startFieldCapacityCalibration();
float calibrateApplicationRate(float before_mm,
                               float after_mm,
                               float irrigationTime_minutes);



#endif
