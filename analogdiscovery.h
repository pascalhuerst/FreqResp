#pragma once

#define TEST 0

#include <list>
#include <vector>
#include <exception>
#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>
#include <cmath>
#include <digilent/waveforms/dwf.h>

#include "debug.h"


class AnalogDiscoveryException : public std::exception {
public:
	AnalogDiscoveryException(std::string func, std::string file, int line, int errorNumber, std::string what);
	virtual const char* what() const noexcept;

private:
	std::string m_func;
	std::string m_file;
	std::string m_msg;
	int m_line;
	int m_errno;
};

class AnalogDiscovery;
typedef std::shared_ptr<AnalogDiscovery> SharedAnalogDiscoveryHandle;

class AnalogDiscovery {
public:
	struct DeviceId {
		int index;
		DEVID id;
		DEVVER ver;
	};

	struct SampleState {
		int available;
		int lost;
		int corrupted;
	};

	AnalogDiscovery(const DeviceId& device);
	~AnalogDiscovery(void);

	std::string version();

	enum Waveform {
		WaveformDc				= 0,
		WaveformSine			= 1,
		WaveformSquare			= 2,
		WaveformTriangle		= 3,
		WaveformRampUp			= 4,
		WaveformRampDown		= 5,
		WaveformNoise			= 6,
		WaveformCustom			= 30,
		WaveformPlay			= 31
	};
	void setAnalogOutputWaveform(int channel, Waveform w);
	void setAnalogOutputAmplitude(int channel, double v);
	void setAnalogOutputFrequency(int channel, double f);
	void setAnalogOutputEnabled(int channel, bool e);

	double setAnalogInputSamplingFreq(double f);
	double analogInputSamplingFreq();
	void setAnalogInputRange(int channel, double v);
	void setAnalogInputEnabled(int channel, bool e);

	enum AcquisitionMode {
		AcquisitionModeSingle			= 0,
		AcquisitionModeScanShift		= 1,
		AcquisitionModeScanScreen		= 2,
		AcquisitionModeRecord			= 3
	};
	void setAnalogInputAcquisitionMode(AcquisitionMode m);
	//TODO: this should be using std::chrono, actually
	double setAnalogInputAcquisitionDuration(double s);
	double analogInputAcquisitionDuration();
	void setAnalogInputReconfigure(bool r);
	void setAnalogInputStart(bool s);
	void setAnalogInputBufferSize(int s);

	enum TriggerSource {
		TriggerSourceNone               = 0,
		TriggerSourcePC                 = 1,
		TriggerSourceDetectorAnalogIn   = 2,
		TriggerSourceDetectorDigitalIn  = 3,
		TriggerSourceAnalogIn           = 4,
		TriggerSourceDigitalIn          = 5,
		TriggerSourceDigitalOut         = 6,
		TriggerSourceAnalogOut1         = 7,
		TriggerSourceAnalogOut2         = 8,
		TriggerSourceAnalogOut3         = 9,
		TriggerSourceAnalogOut4         = 10,
		TriggerSourceExternal1          = 11,
		TriggerSourceExternal2          = 12,
		TriggerSourceExternal3          = 13,
		TriggerSourceExternal4          = 14
	};
	void setAnalogInputTriggerSource(TriggerSource t);
	//TODO: use chrono::duration here
	void setAnalogInputTriggerAutoTimeout(double t);
	void setAnalogInputTriggerChannel(int c);
	enum TriggerType {
		TriggerTypeEdge					= 0,
		TriggerTypePulse				= 1,
		TriggerTypeTransistion			= 2
	};
	void setAnalogInputTriggerType(TriggerType t);
	void setAnalogInputTriggerLevel(double l);
	enum TriggerCondition {
		TriggerConditionRising			= 0,
		TriggerConditionFalling			= 1
	};
	void setAnalogInputTriggerCondition(TriggerCondition c);
	void triggerAnalogInput();

	int analogInputBufferSize();
	bool isOpen(void) const;

	void readAnalogInput(int channel, double *buffer, int size);
	SampleState analogInSampleState();

	enum DeviceState {
		DeviceStateReady				= 0,
		DeviceStateArmed				= 1,
		DeviceStateDone					= 2,
		DeviceStateRunningTriggered		= 3,
		DeviceStateConfig				= 4,
		DeviceStatePrefill				= 5,
		DeviceStateUnknown				= 6,
		DeviceStateWait					= 7
	};
	const static std::vector<std::string> s_stateNames;

	DeviceState analogOutputStatus(int channel);
	DeviceState analogInputStatus(int channel);

	static std::list<DeviceId> getDevices();
	static SharedAnalogDiscoveryHandle createSharedAnalogDiscoveryHandle(AnalogDiscovery::DeviceId deviceId);
	static SharedAnalogDiscoveryHandle getFirstAvailableDevice();
	static void readSamples(SharedAnalogDiscoveryHandle handle, int channel, double *buffer, int bufferSize, std::vector<double> *target, int available);

	// Digital IO
	enum IODirection {
		IODirectionIn = 0x01,
		IODirectionOut = 0x00
	};
	void setDigitalIoDirection(int pin, IODirection d);
	IODirection getDigitalIoDirection(int pin);
	void setDigitalIo(int pin, bool value);
	bool getDigitalIo(int pin);

private:
	HDWF m_devHandle;
	bool m_opened;
	std::string m_version;

	void throwIfNotOpened(std::string func, std::string file, int line);
	void checkAndThrow(bool ret, std::string func, std::string file, int line);
};

std::ostream& operator<<(std::ostream& lhs, const AnalogDiscovery::DeviceState& rhs);
std::ostream& operator<<(std::ostream& lhs, const AnalogDiscovery::SampleState& rhs);
