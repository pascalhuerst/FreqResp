#pragma once

#include "speaker.h"

// Command Line Parameters
const char paramHelp[] = "help,h";
const char paramDebugLevel[] = "debug,d";

const char paramSelfTest[] = "self-test,t";
const char paramManualGpio[] = "manual-gpio,m";
const char paramCalibrate[] = "calibrate,r";

const char paramChannel[] = "channel,c";
const char paramSpeakerChannel[] = "speakerchannel,s";

const char paramfMin[] = "fmin,f";
const char paramfMax[] = "fmax,g";
const char paramPointsPerDecade[] = "points-per-decade,p";
const char paramOutputCalibration[] = "output-calibration,o";


int channel = -1;
Speaker::Channel speakerChannel = Speaker::Lo;
double fMin = 20;			// Default is 20Hz
double fMax = 20000;		// Default is 20kHz
int pointsPerDecade = 20;	// 20 measurements per decade

double outputCalibration = 0.0; // Default 0.0V to have 0dBu @ 1kHz
