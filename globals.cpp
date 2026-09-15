#include "globals.h"
// moisture measurement points parameters
float moisturePoints[4];
int pointIndex = 0;
float avgMoisture = 0;

// ===== Preferences =====
Preferences prefs;

// ===== Calibration =====
float dryRaw = 0;
float wetRaw = 1023;

// ===== Soil Water =====
float SoilWater_mm = 50;
float measured_water=0;
float TAW = 100;
float RAW = 40;


//==========calibration=================
 float FC_percent = 50;
 float PWP_percent = 0;
//===========crop data==================
int ROOT_DEPTH_MM = 300;
float Kc = 1.0;
float P_VALUE = 0.4;
//============ETO==================
float ETO;

float applicationRate = 5;

/* Application rate calibration */
float appRateBefore = 0;
float appRateAfter = 0;
float irrigationTime = 60;

/* Measurement flags */
bool measurementInProgress = false;
bool pointConfirmed = false;

/* Sensor timing */
unsigned long lastSensorSample = 0;
unsigned long sensorStartTime = 0;

/* Commands */
volatile CommandType pendingCommand = CMD_NONE;

/* State machine */
DeviceState currentState = STATE_IDLE;

unsigned long stateStartTime = 0;
