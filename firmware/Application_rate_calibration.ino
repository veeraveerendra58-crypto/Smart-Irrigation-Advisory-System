#include <EEPROM.h>

#define EEPROM_SIZE 64
#define CALIB_ADDR 0

float applicationRate_mm_per_hr = 0.0;

float TAW = 120.0;   // Example Total Available Water (mm)
float rootDepth_cm = 40.0;

unsigned long calibrationStartTime;
bool calibrationRunning = false;

float readMoistureAverage()
{
    float total = 0;
    
    for(int i=0;i<10;i++)
    {
        total += analogRead(34);   // change to your moisture pin logic
        delay(50);
    }

    return total/10.0;
}

float convertToWater_mm(float moisturePercent)
{
    float water = (moisturePercent / 100.0) * TAW;
    return water;
}

void startCalibration()
{
    Serial.println("Calibration Started...");
    calibrationRunning = true;

    float moisturePercent = readMoistureAverage();
    float initialWater_mm = convertToWater_mm(moisturePercent);

    EEPROM.put(8, initialWater_mm);
    EEPROM.commit();

    calibrationStartTime = millis();
}

void stopCalibration()
{
    float irrigationTime_hr = (millis() - calibrationStartTime) / 3600000.0;

    float moisturePercent = readMoistureAverage();
    float finalWater_mm = convertToWater_mm(moisturePercent);

    float initialWater_mm;
    EEPROM.get(8, initialWater_mm);

    float delta_mm = finalWater_mm - initialWater_mm;

    applicationRate_mm_per_hr = delta_mm / irrigationTime_hr;

    EEPROM.put(CALIB_ADDR, applicationRate_mm_per_hr);
    EEPROM.commit();

    calibrationRunning = false;

    Serial.print("Application Rate (mm/hr): ");
    Serial.println(applicationRate_mm_per_hr);
}

float getStoredApplicationRate()
{
    EEPROM.get(CALIB_ADDR, applicationRate_mm_per_hr);
    return applicationRate_mm_per_hr;
}

void setup()
{
    Serial.begin(115200);
    EEPROM.begin(EEPROM_SIZE);

    applicationRate_mm_per_hr = getStoredApplicationRate();

    Serial.print("Stored Application Rate: ");
    Serial.println(applicationRate_mm_per_hr);
}

void loop()
{
    // Example trigger
    if(Serial.read() == 's')
        startCalibration();

    if(Serial.read() == 'e')
        stopCalibration();
}
