#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------- DISPLAY ----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---------- PINS ----------
#define DHTPIN 4
#define DHTTYPE DHT22
#define SOIL_PIN 15
#define LIGHT_PIN 34

// ---------- SETTINGS ----------
#define KC 1.05f
#define ROOT_DEPTH 0.6f
#define SOIL_WHC 0.18f
#define ALTITUDE 100.0f

#define SAMPLE_INTERVAL 10000UL
#define TOTAL_DURATION 1800000UL

#define RAD_A 0.0098f
#define RAD_B -0.85f

DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(SOIL_PIN);
DallasTemperature soilSensor(&oneWire);

float avgT = 0, avgRH = 0, avgLight = 0, avgSoilT = 0;
unsigned long startTime, lastSample = 0;

void drawBar(float percent) {
  int w = (percent / 100.0f) * 108;
  display.drawRect(10, 50, 110, 10, WHITE);
  display.fillRect(11, 51, w, 8, WHITE);
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  soilSensor.begin();
  Wire.begin(21, 22);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(15, 25);
  display.println("ETo Measuring...");
  display.display();
  delay(1500);

  startTime = millis();
}

void loop() {

  unsigned long now = millis();
  unsigned long elapsed = now - startTime;

  if (elapsed < TOTAL_DURATION) {

    if (now - lastSample >= SAMPLE_INTERVAL) {

      float t = dht.readTemperature();
      float h = dht.readHumidity();
      int l = analogRead(LIGHT_PIN);

      soilSensor.requestTemperatures();
      float st = soilSensor.getTempCByIndex(0);

      if (!isnan(t) && !isnan(h) && st > -50) {
        avgT = 0.9f * avgT + 0.1f * t;
        avgRH = 0.9f * avgRH + 0.1f * h;
        avgLight = 0.9f * avgLight + 0.1f * l;
        avgSoilT = 0.9f * avgSoilT + 0.1f * st;
      }

      lastSample = now;
    }

    float percent = (elapsed * 100.0f) / TOTAL_DURATION;

    display.clearDisplay();
    display.setCursor(0, 0);
    display.printf("Air: %.1f C\n", avgT);
    display.printf("Soil: %.1f C\n", avgSoilT);
    display.printf("RH: %.1f %%\n", avgRH);

    display.printf("Progress: %.0f %%\n", percent);
    drawBar(percent);
    display.display();
  }

  else {

    float es = 0.6108f * exp((17.27f * avgT) / (avgT + 237.3f));
    float ea = (avgRH / 100.0f) * es;
    float delta = (4098.0f * es) / ((avgT + 237.3f) * (avgT + 237.3f));

    float P = 101.3f * pow((293.0f - 0.0065f * ALTITUDE) / 293.0f, 5.26f);
    float gamma = 0.000665f * P;

    float Rn = RAD_A * avgLight + RAD_B;

    // ---- Soil Heat Flux ----
    float G = 0.1f * (avgSoilT - avgT);

    float ETo = (0.408f * delta * (Rn - G)) + (0.06f * (es - ea));
    ETo *= 10.0f;

    float ETc = ETo * KC;

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Done");
    display.printf("ETo: %.2f mm\n", ETo);
    display.printf("ETc: %.2f mm\n", ETc);
    display.printf("Water: %.2f L/m2\n", ETc);
    display.display();

    while (1);
  }
}
