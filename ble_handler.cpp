#include "ble_handler.h"
#include "globals.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

static BLECharacteristic *pCharacteristic;
static bool deviceConnected = false;

#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"


/* ================= BLE WRITE CALLBACK ================= */

class MyCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pChar)
    {
        std::string rx = pChar->getValue();
        if(rx.empty()) return;

        StaticJsonDocument<256> doc;

        DeserializationError error = deserializeJson(doc, rx);

        if(error)
        {
            sendBTStatus("JSON Error");
            return;
        }

        String cmd = doc["command"] | "";



        /* ---------- MEASUREMENT ---------- */

        if(cmd == "measure")
        {
            pendingCommand = CMD_START_MEASUREMENT;
            sendBTStatus("Measurement Start");
        }



        /* ---------- MOISTURE POINT ---------- */

        else if(cmd == "point")
        {
            pendingCommand = CMD_CONFIRM_POINT;
            sendBTStatus("Point Confirmed");
        }



        /* ---------- MOISTURE CALIBRATION ---------- */

        else if(cmd == "calibrate_moisture")
        {
            pendingCommand = CMD_CALIBRATE_MOISTURE;
            sendBTStatus("Moisture Calibration Start");
        }



        /* ---------- FIELD CAPACITY ---------- */

        else if(cmd == "calibrate_fc")
        {
            pendingCommand = CMD_CALIBRATE_FC;
            sendBTStatus("FC Calibration Start");
        }



        /* ---------- APPLICATION RATE ---------- */

        else if(cmd == "calibrate_app_rate")
        {
            pendingCommand = CMD_CALIBRATE_APP_RATE;
            sendBTStatus("App Rate Calibration Start");
        }



        /* ---------- STATUS ---------- */

        else if(cmd == "status")
        {
            pendingCommand = CMD_GET_STATUS;
        }



        /* ---------- CROP PARAMETERS JSON ---------- */

        else if(cmd == "crop_params")
        {

            TAW = doc["TAW"] | TAW;
            RAW = doc["RAW"] | RAW;

            prefs.putFloat("TAW", TAW);
            prefs.putFloat("RAW", RAW);

            sendBTStatus("Crop Parameters Saved");
        }



        else
        {
            sendBTStatus("Unknown Command");
        }
    }
};



/* ================= SERVER CALLBACK ================= */

class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer*)
    {
        deviceConnected = true;
    }

    void onDisconnect(BLEServer*)
    {
        deviceConnected = false;
    }
};



/* ================= BLE INIT ================= */

void initBLE()
{

    BLEDevice::init("IrrigationDevice");

    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *service = pServer->createService(SERVICE_UUID);

    pCharacteristic = service->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );

    pCharacteristic->setCallbacks(new MyCallbacks());
    pCharacteristic->addDescriptor(new BLE2902());

    service->start();

    BLEDevice::getAdvertising()->start();
}



/* ================= SEND STATUS ================= */

void sendBTStatus(const String &msg)
{
    if(deviceConnected)
    {
        pCharacteristic->setValue(msg.c_str());
        pCharacteristic->notify();
    }
}
