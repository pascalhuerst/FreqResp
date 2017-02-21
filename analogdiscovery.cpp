#include "analogdiscovery.h"
#include <iostream>
#include <math.h>
#include <unistd.h>

#include <chrono>
#include <thread>

using namespace std;

const std::vector<std::string> AnalogDiscovery::s_stateNames = {
	"Ready",
	"Armed",
	"Done",
	"RunningTriggered",
	"Config",
	"Prefill",
	"Unknown",
	"Wait"
};

AnalogDiscoveryException::AnalogDiscoveryException(std::string func, std::string file, int line, int errorNumber, std::string what) :
	m_func(func),
	m_file(file),
	m_msg(what),
	m_line(line),
	m_errno(errorNumber)
{}

const char* AnalogDiscoveryException::what() const noexcept
{
	return ("AnalogDiscoveryException caught:\n  " + m_file + ":" + std::to_string(m_line) + "\n  " + m_func  + "\n  errno=" + std::to_string(m_errno) + "\n  " + m_msg).c_str();
}

void AnalogDiscovery::throwIfNotOpened(std::string func, std::string file, int line)
{
	if (!m_opened)
		throw AnalogDiscoveryException(func, file, line, 0, "Device not opened");
}

void AnalogDiscovery::checkAndThrow(bool ret, std::string func, std::string file, int line)
{
	if (!ret) {
		DWFERC pdwferc;
		FDwfGetLastError(&pdwferc);

		char szError[512];
		FDwfGetLastErrorMsg(szError);

		throw AnalogDiscoveryException(func, file, line, pdwferc, szError);
	}
}

AnalogDiscovery::AnalogDiscovery(const DeviceId &device)
{
	m_opened = (FDwfDeviceOpen(device.index, &m_devHandle) != 0);

	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	char v[32];
	FDwfGetVersion(v);
	m_version = std::string(v);
}

AnalogDiscovery::~AnalogDiscovery(void)
{
	if (isOpen())
		FDwfDeviceClose(m_devHandle);
}

std::string AnalogDiscovery::version()
{
	return m_version;
}

bool AnalogDiscovery::isOpen(void) const
{
	return m_opened;
}

AnalogDiscovery::DeviceState AnalogDiscovery::analogOutputStatus(int channelId)
{
	DwfState state;
	checkAndThrow(FDwfAnalogOutStatus(m_devHandle, channelId, &state),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
	return static_cast<AnalogDiscovery::DeviceState>(state);
}

AnalogDiscovery::DeviceState AnalogDiscovery::analogInputStatus(int channelId)
{
	DwfState state;
	checkAndThrow(FDwfAnalogInStatus(m_devHandle, channelId, &state),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
	return static_cast<AnalogDiscovery::DeviceState>(state);
}

void AnalogDiscovery::setAnalogOutputWaveform(int channel, AnalogDiscovery::Waveform w)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogOutNodeFunctionSet(m_devHandle, channel, AnalogOutNodeCarrier, static_cast<uint8_t>(w)),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

void AnalogDiscovery::setAnalogOutputAmplitude(int channel, double v)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogOutNodeAmplitudeSet(m_devHandle, channel, AnalogOutNodeCarrier, v),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

}

void AnalogDiscovery::setAnalogOutputEnabled(int channel, bool e)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogOutNodeEnableSet(m_devHandle, channel, AnalogOutNodeCarrier, e),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogOutConfigure(m_devHandle, channel, e),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

void AnalogDiscovery::setAnalogOutputFrequency(int channel, double f)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogOutNodeFrequencySet(m_devHandle, channel, AnalogOutNodeCarrier, f),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}


double AnalogDiscovery::setAnalogInputSamplingFreq(double f)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInFrequencySet(m_devHandle, f),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

	// Stupid API using double instead of int's in millivolts
	double actual = analogInputSamplingFreq();
	if (!(std::fabs(f - actual) < std::numeric_limits<double>::epsilon())) {
		std::string dbg = "Sampling Frequency Differs: desired=" + std::to_string(f) +
				" actual=" + std::to_string(actual);
		Debug::verbose("AnalogDiscovery", dbg);
	}

	return actual;
}

double AnalogDiscovery::analogInputSamplingFreq()
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);
	double f;

	checkAndThrow(FDwfAnalogInFrequencyGet(m_devHandle, &f),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

	return f;
}

void AnalogDiscovery::setAnalogInputRange(int channel, double v)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInChannelRangeSet(m_devHandle, channel, v),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

void AnalogDiscovery::setAnalogInputEnabled(int channel, bool e)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInChannelEnableSet(m_devHandle, channel, e),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

}

void AnalogDiscovery::setAnalogInputAcquisitionMode(AcquisitionMode m)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInAcquisitionModeSet(m_devHandle, static_cast<int>(m)),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

}

double AnalogDiscovery::setAnalogInputAcquisitionDuration(double s)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInRecordLengthSet(m_devHandle, s),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

	// Stupid API using double instead of int's in milliseconds
	double actual = analogInputAcquisitionDuration();
	if (!(std::fabs(s - actual) < std::numeric_limits<double>::epsilon())) {
		std::string dbg = "Acquisition Duration Differs: desired=" + std::to_string(s) +
				" actual=" + std::to_string(actual);
		Debug::verbose("AnalogDiscovery", dbg);
	}
	return actual;
}

double AnalogDiscovery::analogInputAcquisitionDuration()
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	double s;
	checkAndThrow(FDwfAnalogInRecordLengthGet(m_devHandle, &s),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

	return s;
}

void AnalogDiscovery::setAnalogInputReconfigure(bool r)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInConfigure(m_devHandle, r, false),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

}

void AnalogDiscovery::setAnalogInputStart(bool s)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInConfigure(m_devHandle, false, s),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

void AnalogDiscovery::setAnalogInputBufferSize(int s)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInBufferSizeSet(m_devHandle, s),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

void AnalogDiscovery::setAnalogInputTriggerSource(TriggerSource t)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInTriggerSourceSet(m_devHandle, static_cast<unsigned char>(t)),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

void AnalogDiscovery::setAnalogInputTriggerAutoTimeout(double t)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInTriggerAutoTimeoutSet(m_devHandle, t),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

void AnalogDiscovery::setAnalogInputTriggerChannel(int c)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInTriggerChannelSet(m_devHandle, c),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

void AnalogDiscovery::setAnalogInputTriggerType(TriggerType t)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInTriggerTypeSet(m_devHandle, static_cast<int>(t)),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

void AnalogDiscovery::setAnalogInputTriggerLevel(double l)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInTriggerLevelSet(m_devHandle, l),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

void AnalogDiscovery::setAnalogInputTriggerCondition(TriggerCondition t)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInTriggerConditionSet(m_devHandle, static_cast<int>(t)),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

void AnalogDiscovery::triggerAnalogInput()
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfDeviceTriggerPC(m_devHandle),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

int AnalogDiscovery::analogInputBufferSize()
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);
	int size;
	checkAndThrow(FDwfAnalogInBufferSizeInfo(m_devHandle, nullptr, &size),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
	return size;
}

AnalogDiscovery::SampleState AnalogDiscovery::analogInSampleState()
{
	DwfState state = 0;
	checkAndThrow(FDwfAnalogInStatus(m_devHandle, true, &state),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

	AnalogDiscovery::SampleState ret = {-1, -1, -1};

	checkAndThrow(FDwfAnalogInStatusRecord(m_devHandle, &ret.available, &ret.lost, &ret.corrupted),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

	return ret;
}

void AnalogDiscovery::readAnalogInput(int channelId, double *buffer, int size)
{
	checkAndThrow(FDwfAnalogInStatusData(m_devHandle, channelId, buffer, size),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

void AnalogDiscovery::setDigitalIoDirection(int pin, IODirection d)
{
	unsigned int ioMask;
	checkAndThrow(FDwfDigitalIOOutputEnableGet(m_devHandle, &ioMask),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

	unsigned int newBit = 1 << pin;
	// in = 0 / out = 1
	if (d == IODirectionIn)
		ioMask &= ~newBit;
	else
		ioMask |= newBit;

	checkAndThrow(FDwfDigitalIOOutputEnableSet(m_devHandle, ioMask),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

AnalogDiscovery::IODirection AnalogDiscovery::getDigitalIoDirection(int pin)
{
	unsigned int ioMask;
	checkAndThrow(FDwfDigitalIOOutputEnableGet(m_devHandle, &ioMask),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

	return (ioMask & (1 << pin)) ? IODirectionOut : IODirectionIn;
}

void AnalogDiscovery::setDigitalIo(int pin, bool value)
{
	unsigned int ioMask;
	checkAndThrow(FDwfDigitalIOOutputGet(m_devHandle, &ioMask),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

	unsigned int newBit = 1 << pin;
	if (value)
		ioMask |= newBit;
	else
		ioMask &= ~newBit;

	checkAndThrow(FDwfDigitalIOOutputSet(m_devHandle, ioMask),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

bool AnalogDiscovery::getDigitalIo(int pin)
{
	unsigned int ioMask;
	checkAndThrow(FDwfDigitalIOOutputGet(m_devHandle, &ioMask),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

	return ioMask & (1 << pin);
}

//Static
std::list<AnalogDiscovery::DeviceId> AnalogDiscovery::getDevices()
{
	int devCount;
	FDwfEnum(enumfilterAll, &devCount);

	std::list<AnalogDiscovery::DeviceId> ret;
	for (AnalogDiscovery::DeviceId d = {0,0,0}; d.index<devCount; d.index++) {
		FDwfEnumDeviceType(d.index, &d.id, &d.ver);
		ret.push_back(d);
	}
	return ret;
}

//Static
void AnalogDiscovery::readSamples(SharedAnalogDiscoveryHandle handle, int channelId, double *buffer, int bufferSize, std::vector<double> *target, int available)
{
	while (available) {
		int count = available > bufferSize ? bufferSize : available;
		handle->readAnalogInput(channelId, buffer, count);
		copy(&buffer[0], &buffer[count], back_inserter(*target));
		available -= count;
	}

}

//Static
SharedAnalogDiscoveryHandle AnalogDiscovery::createSharedAnalogDiscoveryHandle(AnalogDiscovery::DeviceId deviceId)
{
	return SharedAnalogDiscoveryHandle(new AnalogDiscovery(deviceId));
}

//Static
SharedAnalogDiscoveryHandle AnalogDiscovery::getFirstAvailableDevice()
{
	auto devs = AnalogDiscovery::getDevices();
	if (devs.empty())
		throw AnalogDiscoveryException(__PRETTY_FUNCTION__, __FILE__, __LINE__, 0, "No Analog Discovery devices Found!");

	return createSharedAnalogDiscoveryHandle(devs.front());
}

// Non class functions
std::ostream& operator<<(std::ostream& lhs, const AnalogDiscovery::DeviceState& rhs)
{
	return lhs << AnalogDiscovery::s_stateNames[rhs];
}

std::ostream& operator<<(std::ostream& lhs, const AnalogDiscovery::SampleState& rhs)
{
	return lhs << "available=" << rhs.available
			   << " lost=" << rhs.lost
			   << " corrupted=" << rhs.corrupted;
}
