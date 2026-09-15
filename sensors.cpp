#include "sensors.h"
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define DHTPIN 4
#define DHTTYPE DHT22
#define SOIL_PIN 15
#define LIGHT_PIN 34

static DHT dht(DHTPIN, DHTTYPE);
static OneWire oneWire(SOIL_PIN);
static DallasTemperature soilSensor(&oneWire);

static float avgT = 0;
static float avgRH = 0;
static float avgLight = 0;
static float avgSoilT = 0;

void initSensors()
{
    dht.begin();
    soilSensor.begin();
}

void sampleSensors()
{
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    int l = analogRead(LIGHT_PIN);

    soilSensor.requestTemperatures();
    float st = soilSensor.getTempCByIndex(0);

    if (!isnan(t) && !isnan(h) && st > -50)
    {
        avgT     = 0.9f * avgT     + 0.1f * t;
        avgRH    = 0.9f * avgRH    + 0.1f * h;
        avgLight = 0.9f * avgLight + 0.1f * l;
        avgSoilT = 0.9f * avgSoilT + 0.1f * st;
    }
}

float getAvgTemp()     { return avgT; }
float getAvgHumidity() { return avgRH; }
float getAvgLight()    { return avgLight; }
float getAvgSoilTemp() { return avgSoilT; }
