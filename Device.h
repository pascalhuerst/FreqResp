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
class Indexer
{
public:
	Indexer(T val) : value(val) {}
	static void setStartIndex(int i) { Indexer<T>::index = i; }
	void Print(std::ostream& os) const
	{
		os << (Indexer::index++) << "," << value;
	}
private:
	T value;
	std::string delim;
	static int index;
};

template <class T>
int Indexer<T>::index = 0;

template <class T>
std::ostream& operator<<(std::ostream& lhs, Indexer<T> const& rhs)
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

	bool isOpen(void) const;

	/*
	void enableOutput(double amplitude); // in volts
	void setOutputConfig(double signalFreq);
	void startAcquisition(double voltRange);
	void setInputConfig(double samplingFreq, double samplingDurationS);
	*/

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
	DeviceState inputStatus();

	static int channelId();

	static std::list<DeviceId> getDevices();
	static int chunkSize();

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
	handle->setAnalogInputSamplingFreq(40000);
	handle->setAnalogInputAcquisitionDuration(2.0); //2sec
	handle->setAnalogInputEnabled(Device::channelId(), true);
	handle->setAnalogInputReconfigure(true);

	while(!terminateRequest->load() && pointsIter != points.end()) {

		// Only for debuggin
		sCur = handle->inputStatus();
		if (sLast != sCur) {
			std::cout << "StateChange: " << sLast << " -> " << sCur << std::endl;
			sLast = sCur;
		}

		switch (sCur) {
		case Device::Ready: //Starting State
			handle->setAnalogInputStart(true);

			break;
		case Device::Done:

			std::cout << "f: " << freq << " Hz" << std::endl;
			samplesIter++;	// samples buffer for next frequency
			pointsIter++;	// next frequency
			freq = *pointsIter;

			handle->setAnalogOutputFrequency(Device::channelId(), freq);
			handle->setAnalogOutputEnabled(Device::channelId(), true);


			handle->setAnalogInputStart(true);

			continue;

		case Device::RunningTriggered:

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
