#include "display.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

static void drawBar(float percent)
{
    int w = (percent / 100.0f) * 108;
    display.drawRect(10, 50, 110, 10, WHITE);
    display.fillRect(11, 51, w, 8, WHITE);
}

void initDisplay()
{
    Wire.begin(21, 22);
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
}

void showProgress(float percent, float t, float soilT, float rh)
{
    display.clearDisplay();
    display.setCursor(0, 0);
    display.printf("Air: %.1f C\n", t);
    display.printf("Soil: %.1f C\n", soilT);
    display.printf("RH: %.1f %%\n", rh);
    display.printf("Progress: %.0f %%\n", percent);
    drawBar(percent);
    display.display();
}

void showResult(float eto, float etc)
{
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Done");
    display.printf("ETo: %.2f mm\n", eto);
    display.printf("ETc: %.2f mm\n", etc);
    display.printf("Water: %.2f L/m2\n", etc);
    display.display();
}
