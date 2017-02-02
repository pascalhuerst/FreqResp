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

class AnalogDiscoveryException : public std::exception {
public:
	AnalogDiscoveryException(std::string func, std::string file, int line, int errorNumber, std::string what);
	virtual const char* what() const noexcept;

	std::string m_func;
	std::string m_file;
	std::string m_msg;
	int m_line;
	int m_errno;
private:
};

// Helper, so I can use copy(), to write data with and index values in a csv file;
template <class T>
class IntIndexer
{
public:
	IntIndexer(T val) : value(val) {}
	static void setStartIndex(int i) { IntIndexer<T>::index = i; }
	void Print(std::ostream& os) const
	{
		os << (IntIndexer::index++) << "," << value;
	}
private:
	T value;
	std::string delim;
	static int index;
};

template <class T>
int IntIndexer<T>::index = 0;

template <class T>
std::ostream& operator<<(std::ostream& lhs, IntIndexer<T> const& rhs)
{
	rhs.Print(lhs);
	return lhs;
}

template <typename A, typename B>
class PrintablePair
{
public:
	PrintablePair(A val1, B val2) : first(val1), second(val2) {}
	void Print(std::ostream& os) const
	{
		os << first << "," << second;
	}
	A first;
	B second;
};

template <class T>
std::ostream& operator<<(std::ostream& lhs, PrintablePair<T,T> const& rhs)
{
	rhs.Print(lhs);
	return lhs;
}


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

	void readAnalogInput(double *buffer, int size);
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

	DeviceState analogOutputStatus();
	DeviceState analogInputStatus();

	static int channelId();

	static std::list<DeviceId> getDevices();
	static void readSamples(AnalogDiscovery *handle, double *buffer, int bufferSize, std::vector<double> *target, int available);

	// Digital IO
	enum IODirection {
		IODirectionIn = 0x01,
		IODirectionOut = 0x00
	};
	void setDigitalIoDirection(int pin, IODirection d);



	void setDigitalIo(int pin, bool value);



	enum DebugLevel {
		DebugLevelNone		= 0,
		DebugLevelError		= 1,
		DebugLevelWarning	= 2,
		DebugLevelDebug		= 3,
		DebugLevelVerbose	= 4
	};
	const static std::vector<std::string> s_debugLevelNames;
	static void debug(AnalogDiscovery::DebugLevel level, const std::string& msg);
	static AnalogDiscovery::DebugLevel debugLevel();

private:
	HDWF m_devHandle;
	bool m_opened;
	std::string m_version;

	const static int s_channelId;
	static DebugLevel s_debugLevel;

	void throwIfNotOpened(std::string func, std::string file, int line);
	void checkAndThrow(bool ret, std::string func, std::string file, int line);
};

std::ostream& operator<<(std::ostream& lhs, const AnalogDiscovery::DeviceState& rhs);
std::ostream& operator<<(std::ostream& lhs, const AnalogDiscovery::SampleState& rhs);

typedef std::shared_ptr<std::vector<std::vector<double>>> SharedSampleStorage;
typedef std::shared_ptr<std::atomic<bool>> SharedTerminateFlag;

// Thread function, that polls and reads the inputbuffer
auto readOneBuffer = [](AnalogDiscovery *handle, double currentFrequency)
{
	const double oversampling = 100.0;
	const int desiredSampleCount = 8192;
	const int periodes = 20;
	// Attention: Using for input AND output, but there is a difference
	auto ch = AnalogDiscovery::channelId();
	double *buffer;

	std::vector<double> samples;

	try {

		handle->setAnalogInputEnabled(ch, true);
		handle->setAnalogInputRange(ch, 5);

		handle->setAnalogInputBufferSize(desiredSampleCount);
		int bufferSize = handle->analogInputBufferSize();
		buffer = new double[bufferSize];

		handle->setAnalogInputAcquisitionMode(AnalogDiscovery::AcquisitionModeRecord);

		double desiredSamplingFrequency = oversampling * currentFrequency;
		handle->setAnalogInputSamplingFreq(desiredSamplingFrequency);
		handle->setAnalogInputAcquisitionDuration(1.0 / currentFrequency * periodes);
		handle->setAnalogInputStart(true);

		auto deviceState = AnalogDiscovery::DeviceStateUnknown;

		do {
			auto sampleState = handle->analogInSampleState();
			if (sampleState.corrupted != 0 || sampleState.lost != 0)
				std::cout << sampleState << std::endl;

			if (!sampleState.available)
				deviceState = handle->analogInputStatus();

			if (sampleState.available)
				AnalogDiscovery::readSamples(handle, buffer, bufferSize, &samples, sampleState.available);

		} while (deviceState != AnalogDiscovery::DeviceStateDone);

	} catch (AnalogDiscoveryException e) {

		std::cout << "Cought exception in: " << __PRETTY_FUNCTION__ << std::endl;
		std::cout << "what: " << e.what() << std::endl;
	}

	delete[] buffer;

	return samples;
};

SharedTerminateFlag createTerminateFlag();
