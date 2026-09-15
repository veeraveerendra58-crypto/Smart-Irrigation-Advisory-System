#include "eto.h"
#include <math.h>
#include "globals.h"

#define KC 1.05f
#define ALTITUDE 100.0f
#define RAD_A 0.0098f
#define RAD_B -0.85f

float calculateETo(float avgT, float avgRH, float avgLight, float avgSoilT)
{
    float es = 0.6108f * exp((17.27f * avgT) / (avgT + 237.3f));
    float ea = (avgRH / 100.0f) * es;
    float delta = (4098.0f * es) / ((avgT + 237.3f) * (avgT + 237.3f));

    float P = 101.3f * pow((293.0f - 0.0065f * ALTITUDE) / 293.0f, 5.26f);
    float gamma = 0.000665f * P;

    float Rn = RAD_A * avgLight + RAD_B;

    float G = 0.1f * (avgSoilT - avgT);

    float ETo = (0.408f * delta * (Rn - G)) + (0.06f * (es - ea));
    ETo *= 10.0f;

    return ETo;
}

float calculateETc(float eto,float Kc)
{
    return eto * KC;
}
