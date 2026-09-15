#include "calibration.h"
#include "sensors.h"
#include "config.h"
#include "globals.h"
#include <Arduino.h>

void startMoistureCalibration_dry() {
  dryRaw = averageAnalogRead(MOISTURE_PIN, 10);
  prefs.putFloat("dryRaw", dryRaw);
}
void startMoistureCalibration_wet() {
  wetRaw = averageAnalogRead(MOISTURE_PIN, 10);
  prefs.putFloat("wetRaw", wetRaw);
}

void startFieldCapacityCalibration() {
  int moistureRaw = averageAnalogRead(MOISTURE_PIN, 10);
  FC_percent = rawToMoisturePercent(moistureRaw);

  float availableFraction = (FC_percent - FC_percent * 0.5) / 100.0;
  TAW = availableFraction * ROOT_DEPTH_MM;
  RAW = TAW * P_VALUE;
  SoilWater_mm = TAW;

  prefs.putFloat("TAW", TAW);
  prefs.putFloat("RAW", RAW);
  prefs.putFloat("SoilWater", SoilWater_mm);
}

float calibrateApplicationRate(float before_mm,
                               float after_mm,
                               float irrigationTime_minutes)
{
    if(irrigationTime_minutes <= 0)
        return 0;

    float waterAdded = after_mm - before_mm;

    if(waterAdded <= 0)
        return 0;

    float time_hours = irrigationTime_minutes / 60.0;

    float applicationRate = waterAdded / time_hours;
 
    return applicationRate;   // mm/hr
}
