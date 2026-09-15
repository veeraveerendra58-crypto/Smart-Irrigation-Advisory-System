#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

void initDisplay();
void showProgress(float percent, float t, float soilT, float rh);
void showResult(float eto, float etc);

#endif
