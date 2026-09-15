#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <Preferences.h>
// moisture measuremnt points parameters 
extern float moisturePoints[4];
extern int pointIndex;
extern float avgMoisture;
// ================= PREFERENCES =================
extern Preferences prefs;

// ================= CALIBRATION =================
extern float dryRaw;
extern float wetRaw;

// ================= SOIL WATER PARAMETERS =================
extern float SoilWater_mm;
extern float measured_water;
extern float TAW;
extern float RAW;
//==========calibration=================
extern float FC_percent;
extern float PWP_percent;
//===========crop data==================
extern int ROOT_DEPTH_MM;
extern float Kc;
extern float P_VALUE;
//============ETO===================
extern float ETO;

/* Application rate calibration */
extern float appRateBefore;
extern float appRateAfter;
extern float applicationRate;
extern float irrigationTime;

/* Measurement flags */
extern bool measurementInProgress;
extern bool pointConfirmed;

/* Sensor timing */
extern unsigned long lastSensorSample;
extern unsigned long sensorStartTime;

/* Command enum */

enum CommandType
{
    CMD_NONE,
    CMD_START_MEASUREMENT,
    CMD_CONFIRM_POINT,
    CMD_CALIBRATE_MOISTURE,
    CMD_CALIBRATE_FC,
    CMD_CALIBRATE_APP_RATE,
    CMD_GET_STATUS
};

extern volatile CommandType pendingCommand;

/* Device states */

enum DeviceState
{
    STATE_IDLE,

    STATE_SENSOR_MEASUREMENT,
    STATE_MEASUREMENT_WAIT_POINT,
    STATE_IRRIGATION_CHECK,

    STATE_CALIB_DRY_WAIT,
    STATE_CALIB_WET_WAIT,

    STATE_FC_WAIT,

    STATE_APP_RATE_BEFORE,
    STATE_APP_RATE_AFTER
};

extern DeviceState currentState;

extern unsigned long stateStartTime;

#endif
