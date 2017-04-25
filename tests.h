#pragma once

#include "gpio.h"
#include "measurement.h"
#include <chrono>

void manualGPIOTest();
void manualInputLevelCalibration();
void mapInToOut(SharedGPIOHandle in, SharedGPIOHandle out, std::chrono::milliseconds refreshRate, SharedTerminateFlag terminateRequest);
