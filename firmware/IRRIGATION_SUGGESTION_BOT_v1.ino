#include <Preferences.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "DHT.h"
#include <ArduinoJson.h>

// ==================== HARDWARE CONFIG ====================
#define DHTPIN 4
#define DHTTYPE DHT22
#define MOISTURE_PIN 34
#define SOIL_TEMP_PIN 35
#define LIGHT_PIN 32
#define BUTTON_PIN 25          // Measurement confirmation button

#define NUM_POINTS 4           // Number of soil moisture points per session

// ==================== DEFAULT CROP CONFIG ====================
int ROOT_DEPTH_MM = 300;
float Kc = 1.0;
float P_VALUE = 0.4;

// ==================== GLOBAL VARIABLES ====================
Preferences prefs;
DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(SOIL_TEMP_PIN);
DallasTemperature soilTempSensor(&oneWire);

// Soil calibration
float dryRaw = 0;
float wetRaw = 1023;
float FC_percent = 50;  
float PWP_percent = 0;
float TAW = 0;
float RAW = 0;
float SoilWater_mm = 0;

// BLE
BLEServer *pServer = nullptr;
BLECharacteristic *pCharacteristic = nullptr;
bool deviceConnected = false;

// Measurement session
bool measurementInProgress = false;
int currentPoint = 0;
float moisturePoints[NUM_POINTS];
float avgMoisturePercent = 0;

// Environmental readings
float airTemp=0, humidity=0, lightFactor=0;

// ==================== BUTTON CONFIRMATION FLAG ====================
volatile bool pointConfirmed = false;

// ==================== BLE COMMAND HANDLER ====================
void handleBLECommand(std::string rxValue){
  DynamicJsonDocument doc(512);
  deserializeJson(doc, rxValue);
  String command = doc["command"] | "";

  if(command == "calibrate_moisture") startMoistureCalibration();
  else if(command == "calibrate_fc") startFieldCapacityCalibration();
  else if(command == "configure_crop") configureCrop(doc);
  else if(command == "start_measurement") measurementInProgress = true;
  else if(command == "get_status") sendStatus();
}


// ==================== BLE CALLBACKS ====================
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) { deviceConnected = true; }
  void onDisconnect(BLEServer* pServer) { deviceConnected = false; }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pChar) {
    std::string rxValue = pChar->getValue();
    if (rxValue.length() > 0) handleBLECommand(rxValue);
  }
};

// ==================== HELPER FUNCTIONS ====================
void sendBTStatus(String msg){
  if(deviceConnected){
    pCharacteristic->setValue(msg.c_str());
    pCharacteristic->notify();
  }
}

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

// ==================== CALIBRATION ROUTINES ====================
void startMoistureCalibration(){
  sendBTStatus("Insert sensor in dry soil");
  delay(3000);
  dryRaw = averageAnalogRead(MOISTURE_PIN, 10);
  prefs.putInt("dryRaw", dryRaw);

  sendBTStatus("Insert sensor in wet soil");
  delay(3000);
  wetRaw = averageAnalogRead(MOISTURE_PIN, 10);
  prefs.putInt("wetRaw", wetRaw);

  sendBTStatus("Moisture calibration complete");
}

void startFieldCapacityCalibration(){
  sendBTStatus("Insert sensor in irrigated field after drainage");
  delay(3000);
  int moistureRaw = averageAnalogRead(MOISTURE_PIN, 10);
  FC_percent = rawToMoisturePercent(moistureRaw);
  prefs.putFloat("FC_percent", FC_percent);

  PWP_percent = FC_percent * 0.5;
  float availableFraction = (FC_percent - PWP_percent)/100.0;
  TAW = availableFraction * ROOT_DEPTH_MM;
  RAW = TAW * P_VALUE;
  SoilWater_mm = TAW;

  prefs.putFloat("TAW", TAW);
  prefs.putFloat("RAW", RAW);
  prefs.putFloat("SoilWater", SoilWater_mm);

  sendBTStatus("Field Capacity calibration complete");
}

// ==================== CROP CONFIGURATION ====================
void configureCrop(DynamicJsonDocument &doc){
  String cropName = doc["crop"];
  ROOT_DEPTH_MM = doc["rootDepth_mm"] | ROOT_DEPTH_MM;
  Kc = doc["Kc"] | Kc;
  P_VALUE = doc["p"] | P_VALUE;

  prefs.putString("cropName", cropName);
  prefs.putInt("rootDepth_mm", ROOT_DEPTH_MM);
  prefs.putFloat("Kc", Kc);
  prefs.putFloat("pValue", P_VALUE);

  sendBTStatus("Crop configured: " + cropName);
}


// ==================== STATUS REPORT ====================
void sendStatus(){
  soilTempSensor.requestTemperatures();
  float soilTempC = soilTempSensor.getTempCByIndex(0);
  int moistureRaw = analogRead(MOISTURE_PIN);
  float moisturePercent = rawToMoisturePercent(moistureRaw);

  String status = "Air:" + String(airTemp) +
                  "C Hum:" + String(humidity) +
                  "% SoilT:" + String(soilTempC) +
                  "C Moist:" + String(moisturePercent) +
                  "% AvgSW_mm:" + String(SoilWater_mm);

  sendBTStatus(status);
}

// ==================== SOIL WATER BALANCE ====================
void updateSoilWater(float measuredWater_mm){
  float ETo = 0.5 * (airTemp + 17.8) * lightFactor; // mm/day
  float ETc = ETo * Kc;

  SoilWater_mm -= ETc;

  SoilWater_mm = 0.7*SoilWater_mm + 0.3*measuredWater_mm;

  SoilWater_mm = constrain(SoilWater_mm, 0, TAW);
  prefs.putFloat("SoilWater", SoilWater_mm);
}

// ==================== IRRIGATION CHECK ====================
void checkIrrigation(){
  if(SoilWater_mm <= RAW){
    sendBTStatus("Irrigation needed! Water: " + String(TAW - SoilWater_mm) + " mm");
  } else {
    sendBTStatus("Soil water sufficient");
  }
}

// ==================== BUTTON INTERRUPT ====================
void IRAM_ATTR buttonISR(){
  if(!measurementInProgress){
      measurementInProgress = true;
  } else {
      pointConfirmed = true;
  }
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  prefs.begin("irrigation", false);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

  dht.begin();
  soilTempSensor.begin();

  // Load previous calibration & crop values
  dryRaw = prefs.getInt("dryRaw", 0);
  wetRaw = prefs.getInt("wetRaw", 1023);
  FC_percent = prefs.getFloat("FC_percent", 50);
  TAW = prefs.getFloat("TAW", ROOT_DEPTH_MM * 0.16);
  RAW = prefs.getFloat("RAW", TAW*P_VALUE);
  SoilWater_mm = prefs.getFloat("SoilWater", TAW);

  ROOT_DEPTH_MM = prefs.getInt("rootDepth_mm", ROOT_DEPTH_MM);
  Kc = prefs.getFloat("Kc", Kc);
  P_VALUE = prefs.getFloat("pValue", P_VALUE);

  // BLE Init
  BLEDevice::init("IrrigationDevice");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(BLEUUID((uint16_t)0x180A));
  pCharacteristic = pService->createCharacteristic(
                      BLEUUID((uint16_t)0x2A57),
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->start();
  sendBTStatus("Device Ready");
}

// ==================== MAIN LOOP ====================
void loop() {
  if(measurementInProgress){
    currentPoint = 0;
    // Measure environmental parameters once per session
    airTemp = dht.readTemperature();
    humidity = dht.readHumidity();
    lightFactor = analogRead(LIGHT_PIN)/100000.0;

    while(currentPoint < NUM_POINTS){
      sendBTStatus("Insert sensor at Point " + String(currentPoint+1) + " and press button");

      // Wait for button press
      while(!pointConfirmed){
        delay(100); // small delay to avoid busy loop
      }
      pointConfirmed = false; // reset for next point

      // Read soil moisture
      moisturePoints[currentPoint] = rawToMoisturePercent(averageAnalogRead(MOISTURE_PIN));
      sendBTStatus("Point " + String(currentPoint+1) + " reading: " + String(moisturePoints[currentPoint]) + "%");
      currentPoint++;
    }

    // Average soil moisture
    float sum=0;
    for(int i=0;i<NUM_POINTS;i++) sum += moisturePoints[i];
    avgMoisturePercent = sum/NUM_POINTS;

    float measuredWater_mm = avgMoisturePercent/100.0 * TAW;
    updateSoilWater(measuredWater_mm);
    checkIrrigation();

    measurementInProgress = false;
    sendBTStatus("Measurement complete. Avg moisture: " + String(avgMoisturePercent) + "%");
  }

  delay(500); // main loop delay
}
