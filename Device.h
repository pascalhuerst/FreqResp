#pragma once

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
	bool isOpen(void) const;
	void enableOutput(double amplitude); // in volts
	void setOutputConfig(double signalFreq);

	void startAcquisition(double voltRange);
	void setInputConfig(double sampling_freq, double num_samples);

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

	bool read_rms(double frequency, int num_samples, double& rms_out);

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
typedef std::shared_ptr<std::vector<double>> SharedSampleStorage;
typedef std::shared_ptr<std::atomic<bool>> SharedTerminateFlag;

auto readSamplesFunction = [](Device *handle, SharedSampleStorage samples, SharedTerminateFlag terminateRequest)
{
	double *buffer = new double[Device::chunkSize()];
	double freq = 50;


	while(!terminateRequest->load()) {

		auto state = handle->analogInSampleState();

		if (state.corrupted != 0 || state.lost != 0)
			std::cout << "corrupted=" << state.corrupted << " lost=" << state.lost << std::endl;

		if (handle->inputStatus() == Device::Done) {

			if (freq > 20000) {
				terminateRequest->store(true);
				continue;
			}
			freq *= 2;

			handle->setOutputConfig(freq);
			handle->startAcquisition(10);
		}

		if (state.available > 0) {
			int count = state.available > Device::chunkSize() ? Device::chunkSize() : state.available;
			handle->readAnalogInput(buffer, count);
			copy(&buffer[0], &buffer[count], back_inserter(*samples));
		} else {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	delete[] buffer;
};

SharedTerminateFlag createTerminateFlag();
