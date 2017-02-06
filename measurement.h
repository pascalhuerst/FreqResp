#pragma once

#include <memory>
#include <vector>
#include <map>

#include "analogdiscovery.h"
#include "gpio.h"

typedef std::shared_ptr<std::vector<std::vector<double>>> SharedSampleStorage;
typedef std::shared_ptr<std::atomic<bool>> SharedTerminateFlag;

// Thread function, that polls and reads the inputbuffer
auto readOneBuffer = [](SharedAnalogDiscoveryHandle handle, double currentFrequency)
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

// Helpers, so I can use copy(), to write data with indexed values into a csv file;
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


// GPIO foo

std::list<SharedGPIOHandle> loadDefaultGPIOMapping(SharedAnalogDiscoveryHandle analogDiscovery);

struct GPIOState {
	GPIO::Direction direction;
	bool value;
};
typedef std::map<SharedGPIOHandle, GPIOState> GPIOSnapshot;
GPIOSnapshot createGPIOSnapshot(std::list<SharedGPIOHandle> gpios);
SharedGPIOHandle getGPIOForName(std::list<SharedGPIOHandle> gpios, const std::string &name);
void updateGPIOSnapshot(GPIOSnapshot *snapshot, SharedGPIOHandle gpio, GPIOState state);
void setGPIOSnapshot(GPIOSnapshot snapshot);

// Class

class Measurement
{
public:
	Measurement(const std::string &name, SharedAnalogDiscoveryHandle dev, GPIOSnapshot gpioSnapshot);
	~Measurement();

	void start();
	void stop();
	bool isRunning() const;

private:
	std::string m_name;
	SharedAnalogDiscoveryHandle m_dev;
	GPIOSnapshot m_gpioSnapshot;
	bool m_isRunning;
	SharedTerminateFlag m_terminateRequest;
	std::thread *m_thread;

	double dBuForVolts(double v);
	double dBvForVolts(double v);
	double rms(const std::vector<double>& samples);
	std::vector<double> createMeasuringPoints(int pointsPerDecade, double minHz, double maxHz);
	SharedTerminateFlag createSharedTerminateFlag();
	void saveBuffer(const std::vector<double>& s, const std::string& fileName);

	static void run(SharedTerminateFlag terminateRequest, SharedAnalogDiscoveryHandle dev, Measurement *ptr);

};
