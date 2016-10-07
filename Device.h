#pragma once

#define TEST 0


#include "sample.h"

#include <list>
#include <vector>
#include <exception>
#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>

class DeviceException : public std::exception {
public:
	DeviceException(std::string func, std::string file, int line, int errorNumber, std::string what);
	virtual const char* what() const noexcept;

private:
	std::string m_func;
	std::string m_file;
	int m_line;
	int m_errno;
	std::string m_msg;
};

// Helper, so I can use copy, to write the data using this
// to add index values in csv file;
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


class Device {
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

	Device(const DeviceId& device);
	~Device(void);

	enum Waveform {
		Dc				= 0,
		Sine			= 1,
		Square			= 2,
		Triangle		= 3,
		RampUp			= 4,
		RampDown		= 5,
		Noise			= 6,
		Custom			= 30,
		Play			= 31
	};
	void setAnalogOutputWaveform(int channel, Waveform w);
	void setAnalogOutputAmplitude(int channel, double v);
	void setAnalogOutputFrequency(int channel, double f);
	void setAnalogOutputEnabled(int channel, bool e);

	void setAnalogInputSamplingFreq(double f);
	void setAnalogInputRange(int channel, double v);
	void setAnalogInputEnabled(int channel, bool e);

	enum AcquisitionMode {
		Single			= 0,
		ScanShift		= 1,
		ScanScreen		= 2,
		Record			= 3
	};
	void setAnalogInputAcquisitionMode(AcquisitionMode m);
	//TODO: this should be using std::chrono, actually
	void setAnalogInputAcquisitionDuration(double s);
	void setAnalogInputReconfigure(bool r);
	void setAnalogInputStart(bool s);
	void setAnalogInputBufferSize(int s);

	enum TriggerSource {
		None               = 0,
		PC                 = 1,
		DetectorAnalogIn   = 2,
		DetectorDigitalIn  = 3,
		AnalogIn           = 4,
		DigitalIn          = 5,
		DigitalOut         = 6,
		AnalogOut1         = 7,
		AnalogOut2         = 8,
		AnalogOut3         = 9,
		AnalogOut4         = 10,
		External1          = 11,
		External2          = 12,
		External3          = 13,
		External4          = 14
	};
	void setAnalogInputTriggerSource(TriggerSource t);
	//TODO: use chrono::duration here
	void setAnalogInputTriggerAutoTimeout(double t);
	void setAnalogInputTriggerChannel(int c);
	enum TriggerType {
		Edge				= 0,
		Pulse				= 1,
		Transistion			= 2
	};
	void setAnalogInputTriggerType(TriggerType t);
	void setAnalogInputTriggerLevel(double l);
	enum TriggerCondition {
		Rising				= 0,
		Falling				= 1
	};
	void setAnalogInputTriggerCondition(TriggerCondition c);
	void triggerAnalogInput();


	int analogInputGetBufferSize();


	bool isOpen(void) const;

	void readAnalogInput(double *buffer, int size);
	SampleState analogInSampleState();

	enum DeviceState {
		Ready				= 0,
		Armed				= 1,
		Done				= 2,
		RunningTriggered	= 3,
		Config				= 4,
		Prefill				= 5,
		Unknown				= 6,
		Wait				= 7
	};
	const static std::vector<std::string> s_stateNames;

	DeviceState outputStatus();
	DeviceState analogInputStatus();

	static int channelId();

	static std::list<DeviceId> getDevices();
	static int chunkSize();
	static void readSamples(Device *handle, double *buffer, int bufferSize, std::vector<double> *target, int available);

private:
	HDWF m_devHandle;
	bool m_opened;

	const static int s_channelId;
	const static int s_chunkSize;

	void throwIfNotOpened(std::string func, std::string file, int line);
	void checkAndThrow(bool ret, std::string func, std::string file, int line);
};

std::ostream& operator<<(std::ostream& lhs, const Device::DeviceState& rhs);



// Thread function, that polls and reads the inputbuffer
typedef std::shared_ptr<std::vector<std::vector<double>>> SharedSampleStorage;
typedef std::shared_ptr<std::atomic<bool>> SharedTerminateFlag;



auto readSamplesFunction2 = [](Device *handle,
std::vector<double> points,
SharedSampleStorage samples,
SharedTerminateFlag terminateRequest)
{

	try {

		// Attention: Using for in AND output, but there is a difference
		auto ch = Device::channelId();

		handle->setAnalogInputEnabled(ch, true);
		handle->setAnalogInputRange(ch, 5);
		handle->setAnalogInputSamplingFreq(200000);

		int bufferSize = handle->analogInputGetBufferSize();
		handle->setAnalogInputBufferSize(bufferSize);
		double *buffer = new double[bufferSize];
		std::cout << "Buffersize=" << bufferSize << std::endl;

		//handle->setAnalogInputTriggerSource(Device::None);
		//handle->setAnalogInputTriggerAutoTimeout(0);
		//handle->setAnalogInputTriggerChannel(ch);
		//handle->setAnalogInputTriggerLevel(1.0);
		//handle->setAnalogInputTriggerCondition(Device::Rising);
		//std::this_thread::sleep_for(std::chrono::milliseconds(100));

		handle->setAnalogInputAcquisitionMode(Device::Record);
		handle->setAnalogInputAcquisitionDuration(0.5);

		//std::this_thread::sleep_for(std::chrono::milliseconds(100));

		auto pointsIter = points.begin();	// frequency of measuring point
		auto samplesIter = samples->begin(); // vector for samples of current measuring freq.

		handle->setAnalogOutputAmplitude(ch, 2);
		handle->setAnalogOutputWaveform(ch, Device::Sine);
		handle->setAnalogOutputFrequency(ch, *pointsIter);
		handle->setAnalogOutputEnabled(ch, true);
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));

		handle->setAnalogInputStart(true);

		while(!terminateRequest->load() && pointsIter != points.end()) {

			auto deviceState = handle->analogInputStatus();
			auto sampleState = handle->analogInSampleState();
			if (deviceState == Device::Done) {

				// Probably pending data?
				auto sampleState = handle->analogInSampleState();
				if (sampleState.available) {
					Device::readSamples(handle, buffer, bufferSize, &(*samplesIter), sampleState.available);
					std::cout << "Read another " << sampleState.available << " samples" << std::endl;
				}

				if (sampleState.corrupted != 0 || sampleState.lost != 0)
					std::cout << "corrupted=" << sampleState.corrupted << " lost=" << sampleState.lost << std::endl;

				handle->readAnalogInput(buffer, bufferSize);
				copy(&buffer[0], &buffer[bufferSize], back_inserter(*samplesIter));

				pointsIter++;
				samplesIter++;

				handle->setAnalogOutputFrequency(ch, *pointsIter);
				handle->setAnalogOutputEnabled(0, true);
				//std::this_thread::sleep_for(std::chrono::milliseconds(2000));

				handle->setAnalogInputStart(true);

			} else if (deviceState == Device::RunningTriggered) {

				if (sampleState.corrupted != 0 || sampleState.lost != 0)
					std::cout << "corrupted=" << sampleState.corrupted << " lost=" << sampleState.lost << std::endl;

				if (sampleState.available)
					Device::readSamples(handle, buffer, bufferSize, &(*samplesIter), sampleState.available);

			} else if (deviceState == Device::Prefill) {

			}

		}

		terminateRequest->store(true);
		delete[] buffer;

	} catch (DeviceException e) {

		std::cout << "Cought exception in: " << __PRETTY_FUNCTION__ << std::endl;
		std::cout << "what: " << e.what() << std::endl;

	}
};




////////////////////////////////////////////////////////////////////////////////

auto readSamplesFunction1 = [](Device *handle,
std::vector<double> points,
SharedSampleStorage samples,
SharedTerminateFlag terminateRequest)
{
	double *buffer = new double[Device::chunkSize()];

	auto pointsIter = points.begin();
	auto samplesIter = samples->begin(); // Iter points to a vector of samples

	handle->setAnalogOutputAmplitude(Device::channelId(), 2);
	handle->setAnalogOutputWaveform(Device::channelId(), Device::Sine);

	double freq = *pointsIter;
	handle->setAnalogOutputFrequency(Device::channelId(), freq);
	handle->setAnalogOutputEnabled(Device::channelId(), true);

	Device::DeviceState sCur, sLast = Device::Unknown;

	handle->setAnalogInputAcquisitionMode(Device::Record);
	handle->setAnalogInputRange(Device::channelId(), 5);
	handle->setAnalogInputSamplingFreq(4000);
	handle->setAnalogInputAcquisitionDuration(2.0); //2sec
	handle->setAnalogInputEnabled(Device::channelId(), true);
	handle->setAnalogInputReconfigure(true);

	while(!terminateRequest->load() && pointsIter != points.end()) {

		// Only for debuggin
		sCur = handle->analogInputStatus();
		if (sLast != sCur) {
			std::cout << "StateChange: " << sLast << " -> " << sCur << std::endl;
			sLast = sCur;
		}

		if (sCur == Device::Ready) {

			handle->setAnalogInputStart(true);

		} else if(sCur == Device::Done) {

			std::cout << "f: " << freq << " Hz" << std::endl;
			samplesIter++;	// samples buffer for next frequency
			pointsIter++;	// next frequency
			freq = *pointsIter;

			handle->setAnalogOutputFrequency(Device::channelId(), freq);
			handle->setAnalogOutputEnabled(Device::channelId(), true);


			handle->setAnalogInputStart(true);

			continue;

		} else if (sCur == Device::RunningTriggered) {

			auto sampleState = handle->analogInSampleState();
			if (sampleState.corrupted != 0 || sampleState.lost != 0) {

				static int counter = 0;
				counter++;
				std::cout << "corrupted=" << sampleState.corrupted << " lost=" << sampleState.lost << std::endl;
				//if (counter>5)
				//	exit(-3);
			}

			if (sampleState.available > 0) {

				//std::cout << "samplesIter.size(): " << samplesIter->size() << std::endl;

				int count = sampleState.available > Device::chunkSize() ? Device::chunkSize() : sampleState.available;
				handle->readAnalogInput(buffer, count);
				copy(&buffer[0], &buffer[count], back_inserter(*samplesIter));
			}
			break;
		}


	}

	terminateRequest->store(true);
	delete[] buffer;
};

SharedTerminateFlag createTerminateFlag();
