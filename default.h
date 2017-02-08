#pragma once

// Command Line Parameters
const char paramHelp[] = "help,h";
const char paramSelfTest[] = "self-test,t";
const char paramManualGpio[] = "manual-gpio,m";

const char paramChannelId[] = "channel,c";
const char paramfMin[] = "fmin,f";
const char paramfMax[] = "fmax,g";
const char paramPointsPerDecade[] = "points-per-decade,p";

int channelId = -1;
double fMin = 20;			// Default is 20Hz
double fMax = 20000;		// Default is 20kHz
int pointsPerDecade = 20;// 20 measurements per decade
