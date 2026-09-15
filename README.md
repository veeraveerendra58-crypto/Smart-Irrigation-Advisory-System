# 🌱 Irrigation Advisory System 

> **ESP32-based smart irrigation advisory system** — measures soil moisture and environmental conditions, calculates evapotranspiration, and tells you whether your field needs watering.

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Wiring](#wiring)
- [Installation](#installation)
- [Project Structure](#project-structure)
- [Firmware Architecture](#firmware-architecture)
- [BLE Command Reference](#ble-command-reference)
- [Calibration Guide](#calibration-guide)
- [Measurement Workflow](#measurement-workflow)
- [Configuration & Crop Parameters](#configuration--crop-parameters)
- [Known Issues](#known-issues)
- [Dependencies](#dependencies)
- [License](#license)

---

## Overview

The **Irrigation Suggestion Bot** is an embedded firmware system for the ESP32 that combines:

- Real-time environmental sensing (air temperature, humidity, light, soil temperature)
- Analog soil moisture measurement at multiple field points
- A simplified **Penman-Monteith ETo** (Reference Evapotranspiration) model
- A **soil water balance** that fuses model predictions with live sensor data
- A **Bluetooth Low Energy (BLE)** interface for wireless command and control

After a 30-minute measurement session, the device calculates whether the soil water content has fallen below the **Readily Available Water (RAW)** threshold and sends an irrigation recommendation to a connected BLE client.

---

## Features

| Feature | Detail |
|---|---|
| Sensor sampling | DHT22 + DS18B20 + light + moisture, every 2 s over 30 min |
| Smoothing | First-order exponential moving average (alpha = 0.1) |
| ETo model | Simplified Penman-Monteith (FAO-56 based) |
| Soil water balance | Model + sensor fusion (70 / 30 weighted blend) |
| Calibration | Moisture sensor, field capacity, application rate |
| Communication | BLE (Nordic UART profile, JSON commands) |
| Persistent storage | ESP32 NVS (survives power cycles) |
| Display | SSD1306 128x64 OLED (I2C) |
| Standalone operation | Hardware button triggers measurement without BLE |

---

## Hardware Requirements

| Component | Role |
|---|---|
| ESP32 development board | Microcontroller |
| DHT22 | Air temperature and relative humidity |
| DS18B20 (OneWire) | Soil temperature |
| Resistive / capacitive analog moisture sensor | Volumetric soil moisture |
| LDR or photodiode (analog) | Ambient light proxy for radiation |
| Tactile push button (active LOW) | Standalone trigger / point confirm |
| SSD1306 128x64 OLED | Local display |

> **Note on supply voltage:** The ESP32 is a 3.3 V device. Ensure sensors are compatible or use level shifting as appropriate.

---

## Wiring

> **[PLACEHOLDER: Insert wiring diagram image here (Fritzing / KiCad / schematic)]**

### Pin Map

| Signal | ESP32 GPIO | Notes |
|---|---|---|
| DHT22 Data | GPIO 4 | 10 kOhm pull-up to 3.3 V |
| DS18B20 Data | GPIO 15 | 4.7 kOhm pull-up to 3.3 V |
| Moisture Sensor ADC | GPIO 34 | Input-only; no internal pull-up |
| Light Sensor ADC | GPIO 32 | Defined in config.h |
| Push Button | GPIO 25 | Active LOW; firmware enables INPUT_PULLUP |
| OLED SDA | GPIO 21 | I2C; 4.7 kOhm pull-up |
| OLED SCL | GPIO 22 | I2C; 4.7 kOhm pull-up |

> **Known pin conflict:** `sensors.cpp` redefines `LIGHT_PIN` as GPIO 34, which collides with the moisture sensor. Resolve this before deployment — see [Known Issues](#known-issues).

---

## Installation

### 1. Install Arduino IDE and ESP32 Board Support

```
Tools > Board > Boards Manager > search "esp32" > Install "esp32 by Espressif"
```

### 2. Install Required Libraries

Install the following via **Sketch > Include Library > Manage Libraries**:

| Library | Author |
|---|---|
| DHT sensor library | Adafruit |
| OneWire | Paul Stoffregen |
| DallasTemperature | Miles Burton |
| ArduinoJson | Benoit Blanchon |
| Adafruit GFX Library | Adafruit |
| Adafruit SSD1306 | Adafruit |

> BLE (`BLEDevice`, `BLEServer`), `Preferences`, and `Wire` are included in the ESP32 Arduino core.

### 3. Open and Upload the Sketch

```bash
# Clone or download the repository
git clone [PLACEHOLDER: repository URL]

# Open in Arduino IDE
File > Open > IRRIGATION_SUGGESTION_BOT/IRRIGATION_SUGGESTION_BOT.ino
```

Board settings:
- **Board:** ESP32 Dev Module
- **Upload Speed:** 921600
- **Partition Scheme:** Default 4MB with spiffs

---

## Project Structure

```
IRRIGATION_SUGGESTION_BOT/
├── IRRIGATION_SUGGESTION_BOT.ino   <- Entry point: setup(), loop(), ISR, FSM
├── config.h                         <- Pin definitions (edit before flashing)
├── globals.h / globals.cpp          <- Shared state, command & state enums
├── sensors.h / sensors.cpp          <- DHT22 + DS18B20 sampling + EMA filter
├── moisture.h / moisture.cpp        <- Analog moisture reading & conversion
├── calibration.h / calibration.cpp  <- Dry/wet/FC/app-rate calibration
├── eto.h / eto.cpp                  <- Penman-Monteith ETo & ETc calculation
├── irrigation.h / irrigation.cpp    <- Soil water balance & irrigation decision
├── ble_handler.h / ble_handler.cpp  <- BLE server, JSON command parser
└── display.h / display.cpp          <- SSD1306 OLED rendering
```

---

## Firmware Architecture

### Main Loop

```cpp
void loop() {
    handleCommands();   // Process any pending BLE / button command
    runStateMachine();  // Execute current state logic
}
```

Commands are deposited into `pendingCommand` by either the BLE write callback or the hardware button ISR, and are consumed safely in the main loop.

```cpp
// Button ISR — stored in IRAM for fast execution
void IRAM_ATTR buttonISR() {
    if (!measurementInProgress)
        pendingCommand = CMD_START_MEASUREMENT;
    else
        pointConfirmed = true;
}
```

### State Machine

```
             [measure / button]
                    |
              STATE_IDLE <--------------------------+
                    |                               |
                    v                               |
        STATE_SENSOR_MEASUREMENT                    |
         (sample every 2 s, 30 min)                 |
                    |  30 min elapsed               |
                    v                               |
        STATE_MEASUREMENT_WAIT_POINT                |
         (await 4 confirmed moisture points)        |
                    |  4 points confirmed           |
                    v                               |
         STATE_IRRIGATION_CHECK                     |
          calculateETo()                            |
          updateSoilWater()                         |
          irrigationNeeded()                        |
                    +-------------------------------+

Calibration states (entered from IDLE, all return to IDLE):
  STATE_CALIB_DRY_WAIT  -> STATE_CALIB_WET_WAIT  -> IDLE
  STATE_FC_WAIT                                   -> IDLE
  STATE_APP_RATE_BEFORE -> STATE_APP_RATE_AFTER   -> IDLE
```

### Sensor Smoothing

```cpp
// All four sensor values use this exponential moving average
// Alpha = 0.1 means time constant ~19 samples (38 s at 2 s interval)
avgT     = 0.9f * avgT     + 0.1f * t;
avgRH    = 0.9f * avgRH    + 0.1f * h;
avgLight = 0.9f * avgLight + 0.1f * l;
avgSoilT = 0.9f * avgSoilT + 0.1f * st;
```

### ETo Formula (Penman-Monteith, simplified)

```cpp
// Saturation vapour pressure (kPa)
float es    = 0.6108f * exp((17.27f * avgT) / (avgT + 237.3f));
// Actual vapour pressure (kPa)
float ea    = (avgRH / 100.0f) * es;
// Slope of vapour pressure curve (kPa/degC)
float delta = (4098.0f * es) / ((avgT + 237.3f) * (avgT + 237.3f));
// Net radiation proxy from ADC (requires site calibration!)
float Rn    = RAD_A * avgLight + RAD_B;
// Soil heat flux proxy
float G     = 0.1f * (avgSoilT - avgT);
// Reference ETo (wind term simplified to constant 0.06)
float ETo   = (0.408f * delta * (Rn - G)) + (0.06f * (es - ea));
ETo *= 10.0f;
```

### Soil Water Balance

```cpp
void updateSoilWater(float measuredWater_mm) {
    float etc     = calculateETc(ETO, Kc);
    SoilWater_mm -= etc;                                           // subtract crop water use
    SoilWater_mm  = 0.7f * SoilWater_mm + 0.3f * measuredWater_mm; // fuse model + sensor
    SoilWater_mm  = constrain(SoilWater_mm, 0, TAW);               // physical clamp
}

bool irrigationNeeded() {
    return SoilWater_mm <= RAW;
}
```

---

## BLE Command Reference

Connect to the device named **`IrrigationDevice`**.

| Property | Value |
|---|---|
| Service UUID | `6E400001-B5A3-F393-E0A9-E50E24DCCA9E` |
| Characteristic UUID | `6E400002-B5A3-F393-E0A9-E50E24DCCA9E` |
| Write format | UTF-8 JSON |
| Notify format | UTF-8 plain text |

### Commands

| Command | JSON Payload | Description |
|---|---|---|
| Start measurement | `{"command":"measure"}` | Starts 30-min sensor session |
| Confirm point | `{"command":"point"}` | Records a moisture sampling point |
| Calibrate moisture | `{"command":"calibrate_moisture"}` | Runs dry then wet sensor calibration |
| Calibrate field capacity | `{"command":"calibrate_fc"}` | Records field capacity moisture |
| Calibrate app rate | `{"command":"calibrate_app_rate"}` | Measures irrigation application rate |
| Set crop parameters | `{"command":"crop_params","TAW":90,"RAW":36}` | Updates TAW and RAW, saves to NVS |
| Status *(reserved)* | `{"command":"status"}` | No-op — not yet implemented |

### Notifications

```
"Starting 30min measurement"
"Point Recorded"
"All Points Measured"
"Moisture Calibration Done"
"Field Capacity Saved"
"Crop Parameters Saved"
"Calibration Completed"
"JSON Error"
```

---

## Calibration Guide

Perform these steps in order before first deployment.

### Step 1 — Moisture Sensor Calibration

```
Send: {"command":"calibrate_moisture"}
  Step A: Insert sensor in completely DRY soil (or hold in open air)
          -> Device waits 3 s then records dryRaw
  Step B: Insert sensor in SATURATED soil
          -> Device waits 3 s then records wetRaw
  Receive: "Moisture Calibration Done"
```

### Step 2 — Field Capacity Calibration

```
1. Saturate the soil, then allow to drain freely for 24-48 hours
2. Insert moisture sensor into soil at field capacity
3. Send: {"command":"calibrate_fc"}
4. Send: {"command":"point"}  (or press button)
5. Receive: "Field Capacity Saved"
   -> TAW, RAW, and initial SoilWater_mm are calculated and saved
```

### Step 3 — Application Rate Calibration

```
1. Send: {"command":"calibrate_app_rate"}
2. Confirm 4 pre-irrigation points (send "point" x 4)
3. Run irrigation for exactly [irrigationTime] minutes (default: 60 min)
4. Confirm 4 post-irrigation points (send "point" x 4)
5. Receive: "Calibration Completed"  -> applicationRate (mm/hr) saved
```

---

## Measurement Workflow

```
1. TRIGGER   Send {"command":"measure"} or press the hardware button
             |
2. 30 MIN    Sensors sampled every 2 s, EMA averages updated continuously
             |
3. POINTS    "Confirm Moisture Points" notification received
             -> Move probe to 4 field locations, send "point" at each
             |
4. CALCULATE ETo calculated from 30-min averages
             Soil water balance updated: SoilWater_mm fused with sensor
             |
5. RESULT    BLE notification: irrigation needed true / false
             OLED shows ETo and ETc values
```

---

## Configuration & Crop Parameters

Send `crop_params` via BLE to update TAW and RAW without reflashing:

```json
{"command":"crop_params","TAW":90,"RAW":36}
```

For parameters not yet exposed via BLE, edit `globals.cpp` and reflash:

| Variable | Default | Description |
|---|---|---|
| `ROOT_DEPTH_MM` | 300 mm | Root zone depth |
| `Kc` | 1.0 | FAO-56 crop coefficient |
| `P_VALUE` | 0.4 | Depletion fraction p |
| `irrigationTime` | 60 min | Irrigation duration for rate calibration |

> **[PLACEHOLDER: Add a table of Kc values and recommended P for your target crops here]**

---

## Known Issues

| ID | Severity | Description |
|---|---|---|
| BUG-01 | HIGH | `sensors.cpp` redefines `LIGHT_PIN` as GPIO 34, colliding with `MOISTURE_PIN` |
| BUG-02 | HIGH | `SOIL_TEMP_PIN` (GPIO 35) in `config.h` is unused; `sensors.cpp` uses GPIO 15 |
| BUG-03 | HIGH | `#define KC 1.05` in `eto.cpp` shadows global `Kc`; user-set crop coefficient is ignored in ETc |
| BUG-04 | MEDIUM | `initDisplay()` is never called in `setup()`; OLED stays blank |
| BUG-05 | MEDIUM | `averageAnalogRead()` duplicated in `sensors.cpp` and `moisture.cpp` |
| BUG-06 | MEDIUM | `status` BLE command is a no-op (handler commented out) |
| BUG-07 | LOW | `irrigationTime` cannot be configured via BLE |
| BUG-08 | LOW | `RAD_A` / `RAD_B` radiation proxy coefficients are unvalidated — ETo accuracy requires site calibration |

---

## Dependencies

| Library | Min Version | Install |
|---|---|---|
| DHT sensor library | 1.4 | Arduino Library Manager |
| OneWire | 2.3 | Arduino Library Manager |
| DallasTemperature | 3.9 | Arduino Library Manager |
| ArduinoJson | 6.21 | Arduino Library Manager |
| Adafruit GFX Library | 1.11 | Arduino Library Manager |
| Adafruit SSD1306 | 2.5 | Arduino Library Manager |
| ESP32 BLE Arduino | built-in | ESP32 Arduino core |
| Preferences | built-in | ESP32 Arduino core |
| Wire | built-in | Arduino core |

---
