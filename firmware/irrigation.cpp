#include "irrigation.h"
#include "eto.h"
#include "calibration.h"
#include "globals.h"
#include <Arduino.h>


void updateSoilWater( float measuredWater_mm) {

 
  float etc = calculateETc(ETO, Kc);

  SoilWater_mm -= etc;

  SoilWater_mm = 0.7 * SoilWater_mm + 0.3 * measuredWater_mm;

  SoilWater_mm = constrain(SoilWater_mm, 0, TAW);
}
bool irrigationNeeded()
{
    return SoilWater_mm <= RAW;
}
