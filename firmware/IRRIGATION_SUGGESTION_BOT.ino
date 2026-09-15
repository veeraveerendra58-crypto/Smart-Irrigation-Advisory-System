#include "config.h"
#include "sensors.h"
#include "calibration.h"
#include "irrigation.h"
#include "ble_handler.h"
#include "moisture.h"
#include "globals.h"
#include "eto.h"

#define SENSOR_INTERVAL 2000
#define SENSOR_MEASURE_TIME 1800000

void IRAM_ATTR buttonISR()
{
    if(!measurementInProgress)
        pendingCommand = CMD_START_MEASUREMENT;
    else
        pointConfirmed = true;
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
  
  Serial.begin(115200);
  initSensors();
  initBLE();
    prefs.begin("irrigation", false);
    FC_percent = prefs.getFloat("FC_percent", 50);
    dryRaw = prefs.getFloat("dryRaw", 0);
    wetRaw = prefs.getFloat("wetRaw", 1023);
    TAW = prefs.getFloat("TAW", 100);
    RAW = prefs.getFloat("RAW", 40);
    SoilWater_mm = prefs.getFloat("SoilWater", TAW);
    ROOT_DEPTH_MM = prefs.getInt("rootDepth_mm", ROOT_DEPTH_MM);
    Kc = prefs.getFloat("Kc", Kc);
    P_VALUE = prefs.getFloat("pValue", P_VALUE);
    applicationRate = prefs.getFloat("applicationRate", applicationRate);
}
void handleCommands()
{
    switch(pendingCommand)
    {

        case CMD_START_MEASUREMENT:

            measurementInProgress = true;
            sensorStartTime = millis();

            currentState = STATE_SENSOR_MEASUREMENT;

            sendBTStatus("Starting 30min measurement");

            pendingCommand = CMD_NONE;
        break;


        case CMD_CONFIRM_POINT:

            pointConfirmed = true;

            pendingCommand = CMD_NONE;
        break;


        case CMD_CALIBRATE_MOISTURE:

            currentState = STATE_CALIB_DRY_WAIT;

            stateStartTime = millis();

            sendBTStatus("Insert Dry Soil");

            pendingCommand = CMD_NONE;
        break;


        case CMD_CALIBRATE_FC:

            currentState = STATE_FC_WAIT;

            sendBTStatus("Insert Field Capacity Soil");

            pendingCommand = CMD_NONE;
        break;


        case CMD_CALIBRATE_APP_RATE:

            currentState = STATE_APP_RATE_BEFORE;

            sendBTStatus("Before Irrigation Reading");

            pendingCommand = CMD_NONE;
        break;


        case CMD_GET_STATUS:

            //checkIrrigation();

            pendingCommand = CMD_NONE;
        break;

        default:
        break;
    }
}
void runStateMachine()
{

switch(currentState)
{

case STATE_IDLE:
break;



/* 30 MIN SENSOR MEASUREMENT */

case STATE_SENSOR_MEASUREMENT:

    if(millis() - lastSensorSample >= SENSOR_INTERVAL)
    {
        lastSensorSample = millis();
        sampleSensors();
    }

    if(millis() - sensorStartTime >= SENSOR_MEASURE_TIME)
    {
        measurementInProgress = false;

        currentState = STATE_MEASUREMENT_WAIT_POINT;

        sendBTStatus("Confirm Moisture Points");
    }

break;



/* MOISTURE POINT COLLECTION */

case STATE_MEASUREMENT_WAIT_POINT:

    if(pointConfirmed)
    {
        pointConfirmed = false;

        
        float percent = readMoisturePoint();

        moisturePoints[pointIndex] = percent;

        pointIndex++;

        sendBTStatus("Point Recorded");

        if(pointIndex >= 4)
        {
            float sum = 0;

            for(int i=0;i<4;i++)
                sum += moisturePoints[i];

            avgMoisture = sum / 4.0;
            
            measured_water=convertToWaterMM(avgMoisture); 

            pointIndex = 0;

            sendBTStatus("All Points Measured");

            currentState = STATE_IRRIGATION_CHECK;
        }
    }

break;



/* MOISTURE CALIBRATION */

case STATE_CALIB_DRY_WAIT:

    if(millis() - stateStartTime > 3000)
    {
        startMoistureCalibration_dry();
       
        currentState = STATE_CALIB_WET_WAIT;

        stateStartTime = millis();

        sendBTStatus("Insert Wet Soil");
    }

break;



case STATE_CALIB_WET_WAIT:

    if(millis() - stateStartTime > 3000)
    {
        startMoistureCalibration_wet();

        sendBTStatus("Moisture Calibration Done"); 

        currentState = STATE_IDLE;
    }

break;



/* FIELD CAPACITY CALIBRATION */

case STATE_FC_WAIT:

    if(pointConfirmed)
    {
        pointConfirmed = false;

       startFieldCapacityCalibration();

        sendBTStatus("Field Capacity Saved");

        currentState = STATE_IDLE;
    }

break;



/* APPLICATION RATE BEFORE IRRIGATION */

case STATE_APP_RATE_BEFORE:

    if(pointConfirmed)
    {
 pointConfirmed = false;

        
        float percent = readMoisturePoint();

        moisturePoints[pointIndex] = percent;

        pointIndex++;

        sendBTStatus("Point Recorded");

        if(pointIndex >= 4)
        {
            float sum = 0;

            for(int i=0;i<4;i++)
                sum += moisturePoints[i];

            avgMoisture = sum / 4.0;

            appRateBefore = convertToWaterMM(avgMoisture);

            pointIndex = 0;

            sendBTStatus("All Points Measured");

            currentState = STATE_APP_RATE_AFTER;
    }
    }

break;



/* APPLICATION RATE AFTER IRRIGATION */

case STATE_APP_RATE_AFTER:

    if(pointConfirmed)
    {
       
        pointConfirmed = false;

        
        float percent = readMoisturePoint();

        moisturePoints[pointIndex] = percent;

        pointIndex++;

        sendBTStatus("Point Recorded");

        if(pointIndex >= 4)
        {
            float sum = 0;

            for(int i=0;i<4;i++)
                sum += moisturePoints[i];

            avgMoisture = sum / 4.0;

            appRateAfter = convertToWaterMM(avgMoisture);

            pointIndex = 0;

            sendBTStatus("All Points Measured");
             applicationRate= calibrateApplicationRate(appRateBefore,appRateAfter, irrigationTime);
            prefs.putFloat("applicationRate", applicationRate);
            sendBTStatus("Calibration Completed");
        currentState = STATE_IDLE;
    }
    }
break;



case STATE_IRRIGATION_CHECK:
    calculateETo(getAvgTemp(), getAvgHumidity(), getAvgLight(), getAvgSoilTemp());
    updateSoilWater( measured_water);
    irrigationNeeded();

    currentState = STATE_IDLE;

break;

}

} 


void loop()
{
    handleCommands();

    runStateMachine();
}
