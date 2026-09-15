#include "moisture.h"

#include "globals.h"

#define MOISTURE_PIN 34



int averageAnalogRead(int pin, int samples=10){
  long sum = 0;
  for(int i=0;i<samples;i++){
    sum += analogRead(pin);
    delay(50);
  }
  return sum / samples;
}

float rawToMoisturePercent(int raw){
  float percent = ((float)(raw - dryRaw))*100.0/(wetRaw - dryRaw);
  if(percent < 0) percent = 0;
  if(percent > 100) percent = 100;
  return percent;
}

void initMoisture(){
  prefs.begin("irrigation", false);
  loadMoisturePreferences();
}

void loadMoisturePreferences(){
  dryRaw = prefs.getInt("dryRaw", 0);
  wetRaw = prefs.getInt("wetRaw", 1023);
  FC_percent = prefs.getFloat("FC_percent", 50);

  ROOT_DEPTH_MM = prefs.getInt("rootDepth_mm", ROOT_DEPTH_MM);
  Kc = prefs.getFloat("Kc", Kc);
  P_VALUE = prefs.getFloat("pValue", P_VALUE);

  TAW = prefs.getFloat("TAW", ROOT_DEPTH_MM * 0.16);
  RAW = prefs.getFloat("RAW", TAW * P_VALUE);
  SoilWater_mm = prefs.getFloat("SoilWater", TAW);
}

float readMoisturePoint(){
  int raw = averageAnalogRead(MOISTURE_PIN);
  return rawToMoisturePercent(raw);
}

float computeAverageMoisture(float points[], int count){
  float sum = 0;
  for(int i=0;i<count;i++)
    sum += points[i];
  return sum / count;
}

float convertToWaterMM(float moisturePercent){
  return (moisturePercent/100.0f) * TAW;
}

// ================= Calibration =================

/*void startMoistureCalibration(){
  delay(3000);
  dryRaw = averageAnalogRead(MOISTURE_PIN);
  prefs.putInt("dryRaw", dryRaw);

  delay(3000);
  wetRaw = averageAnalogRead(MOISTURE_PIN);
  prefs.putInt("wetRaw", wetRaw);
}


void startFieldCapacityCalibration(){
  delay(3000);

  int raw = averageAnalogRead(MOISTURE_PIN);
  FC_percent = rawToMoisturePercent(raw);
  prefs.putFloat("FC_percent", FC_percent);

  PWP_percent = FC_percent * 0.5;

  float availableFraction = (FC_percent - PWP_percent)/100.0;
  TAW = availableFraction * ROOT_DEPTH_MM;
  RAW = TAW * P_VALUE;
  SoilWater_mm = TAW;

  prefs.putFloat("TAW", TAW);
  prefs.putFloat("RAW", RAW);
  prefs.putFloat("SoilWater", SoilWater_mm);
}
*/
