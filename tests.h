#pragma once

#include "gpio.h"
#include "measurement.h"

void manualGPIOTest();
void manualInputLevelCalibration();
void mapInToOut(SharedGPIOHandle in, SharedGPIOHandle out, int refreshRate, SharedTerminateFlag terminateRequest);



