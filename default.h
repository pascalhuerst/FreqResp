#pragma once

#include "speaker.h"

// Command Line Parameters
const char paramHelp[] = "help";
const char paramDebugLevel[] = "debug";

const char paramSelfTest[] = "self-test";
const char paramManualGpio[] = "manual-gpio";
const char paramCalibrate[] = "calibrate";

const char paramListGpios[] = "list-gpios";
const char paramSetGpios[] = "set-gpios";


const char paramChannel[] = "channel";
const char paramSpeakerChannel[] = "speakerchannel";

const char paramfMin[] = "fmin";
const char paramfMax[] = "fmax";
const char paramPointsPerDecade[] = "points-per-decade";
const char paramOutputCalibration[] = "output-calibration";

const char paramOutputFile[] = "output";


int channel = -1;
Speaker::Channel speakerChannel = Speaker::Lo;
double fMin = 20;			// Default is 20Hz
double fMax = 20000;		// Default is 20kHz
int pointsPerDecade = 20;	// 20 measurements per decade

double outputCalibration = 0.0; // Default 0.0V to have 0dBu @ 1kHz
std::string  outputName = "MyMeasurement";
