// ===== SOIL PROFILE IRRIGATION PROBE =====
// ESP32 Version

// -------- PIN DEFINITIONS --------
#define S1_PIN 34
#define S2_PIN 35
#define S3_PIN 32
#define S4_PIN 33

// -------- USER SETTINGS --------
float rootDepth_m = 0.60;        // Adjustable root depth (meters)
float layerThickness = 0.20;     // 20 cm per layer
float FC_minus_WP = 0.18;        // For loam soil
int samples = 30;                // Averaging samples

// -------- CALIBRATION VALUES --------
// Replace with your measured dry/wet ADC values
int dryCal[4]  = {3200, 3200, 3200, 3200};
int wetCal[4]  = {1500, 1500, 1500, 1500};

// -------- GLOBAL VARIABLES --------
float TAW_layer;
float moisturePercent[4];
float layerWater[4];

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  TAW_layer = 1000 * FC_minus_WP * layerThickness;

  Serial.println("Portable Soil Profile Probe Started");
}

void loop() {

  int raw[4];

  raw[0] = readAveraged(S1_PIN, 0);
  raw[1] = readAveraged(S2_PIN, 1);
  raw[2] = readAveraged(S3_PIN, 2);
  raw[3] = readAveraged(S4_PIN, 3);

  float totalWater = 0;
  float totalTAW = 0;

  for (int i = 0; i < 4; i++) {

    moisturePercent[i] = mapToPercent(raw[i], i);

    float moistureFraction = moisturePercent[i] / 100.0;
    layerWater[i] = moistureFraction * TAW_layer;

    float layerTop = i * layerThickness;
    float layerBottom = layerTop + layerThickness;

    float effectiveDepth = min(layerBottom, rootDepth_m) - layerTop;

    if (effectiveDepth > 0) {
      float coverageFactor = effectiveDepth / layerThickness;
      totalWater += layerWater[i] * coverageFactor;
      totalTAW += TAW_layer * coverageFactor;
    }
  }

  float depletion = totalTAW - totalWater;

  Serial.println("------ PROFILE DATA ------");
  Serial.print("Total Water (mm): ");
  Serial.println(totalWater, 2);

  Serial.print("Total TAW (mm): ");
  Serial.println(totalTAW, 2);

  Serial.print("Depletion (mm): ");
  Serial.println(depletion, 2);

  if (depletion > (0.5 * totalTAW)) {
    Serial.println("Irrigation Recommended");
    Serial.print("Apply Water (mm): ");
    Serial.println(depletion, 2);
  } else {
    Serial.println("No Irrigation Needed");
  }

  Serial.println("--------------------------\n");

  delay(10000); // portable reading every 10 sec
}

// -------- FUNCTIONS --------

int readAveraged(int pin, int index) {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delay(5);
  }
  return sum / samples;
}

float mapToPercent(int raw, int index) {

  float percent = (float)(raw - dryCal[index]) * 100.0 /
                  (wetCal[index] - dryCal[index]);

  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;

  return percent;
}
