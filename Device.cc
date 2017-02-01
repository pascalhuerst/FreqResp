#include "Device.h"
#include <iostream>
#include <math.h>
#include <unistd.h>

#include <chrono>
#include <thread>

using namespace std;

const int Device::s_channelId = 0;
Device::DebugLevel Device::s_debugLevel = Device::DebugLevelWarning;

const std::vector<std::string> Device::s_stateNames = {
	"Ready",
	"Armed",
	"Done",
	"RunningTriggered",
	"Config",
	"Prefill",
	"Unknown",
	"Wait"
};

const std::vector<std::string> Device::s_debugLevelNames = {
	"None   ",
	"Error  ",
	"Warning",
	"Debug  ",
	"Verbose"
};

DeviceException::DeviceException(std::string func, std::string file, int line, int errorNumber, std::string what) :
	m_func(func),
	m_file(file),
	m_msg(what),
	m_line(line),
  m_errno(errorNumber)
{}

const char* DeviceException::what() const noexcept
{
	return (m_file + ":" + m_func + ":" + std::to_string(m_line) + "(" + std::to_string(m_errno) + ")" + "\n   " + m_msg).c_str();
}

void Device::throwIfNotOpened(std::string func, std::string file, int line)
{
	if (!m_opened)
		throw DeviceException(func, file, line, 0, "Device not opened");
}

void Device::checkAndThrow(bool ret, std::string func, std::string file, int line)
{
	if (!ret) {
		DWFERC pdwferc;
		FDwfGetLastError(&pdwferc);

		char szError[512];
		FDwfGetLastErrorMsg(szError);

		throw DeviceException(func, file, line, pdwferc, szError);
	}
}

Device::Device(const DeviceId &device)
{
	m_opened = (FDwfDeviceOpen(device.index, &m_devHandle) != 0);

	char v[32];

	if (m_opened)
		FDwfGetVersion(v);

	m_version = std::string(v);
}

Device::~Device(void)
{
	if (isOpen())
		FDwfDeviceClose(m_devHandle);
}

std::string Device::version()
{
	return m_version;
}

bool Device::isOpen(void) const
{
	return m_opened;
}

Device::DeviceState Device::analogOutputStatus()
{
	DwfState state;
	checkAndThrow(FDwfAnalogOutStatus(m_devHandle, s_channelId, &state),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
	return static_cast<Device::DeviceState>(state);
}

Device::DeviceState Device::analogInputStatus()
{
	DwfState state;
	checkAndThrow(FDwfAnalogInStatus(m_devHandle, s_channelId, &state),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
	return static_cast<Device::DeviceState>(state);
}




void Device::setAnalogOutputWaveform(int channel, Device::Waveform w)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogOutNodeFunctionSet(m_devHandle, channel, AnalogOutNodeCarrier, static_cast<uint8_t>(w)),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

void Device::setAnalogOutputAmplitude(int channel, double v)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogOutNodeAmplitudeSet(m_devHandle, channel, AnalogOutNodeCarrier, v),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

}

void Device::setAnalogOutputEnabled(int channel, bool e)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogOutNodeEnableSet(m_devHandle, channel, AnalogOutNodeCarrier, e),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogOutConfigure(m_devHandle, channel, e),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

void Device::setAnalogOutputFrequency(int channel, double f)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogOutNodeFrequencySet(m_devHandle, channel, AnalogOutNodeCarrier, f),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}


double Device::setAnalogInputSamplingFreq(double f)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInFrequencySet(m_devHandle, f),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

	// Stupid API using double instead of int's in millivolts
	double actual = analogInputSamplingFreq();
	if (!(std::fabs(f - actual) < std::numeric_limits<double>::epsilon())) {
		std::string dbg = "Sampling Frequency Differs: desired=" + std::to_string(f) +
					 " actual=" + std::to_string(actual);
		debug(Device::DebugLevelDebug, dbg);
	}

	return actual;
}

double Device::analogInputSamplingFreq()
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);
	double f;

	checkAndThrow(FDwfAnalogInFrequencyGet(m_devHandle, &f),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

	return f;
}

void Device::setAnalogInputRange(int channel, double v)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInChannelRangeSet(m_devHandle, channel, v),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

void Device::setAnalogInputEnabled(int channel, bool e)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInChannelEnableSet(m_devHandle, channel, e),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

}

void Device::setAnalogInputAcquisitionMode(AcquisitionMode m)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInAcquisitionModeSet(m_devHandle, static_cast<int>(m)),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

}

double Device::setAnalogInputAcquisitionDuration(double s)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInRecordLengthSet(m_devHandle, s),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

	// Stupid API using double instead of int's in milliseconds
	double actual = analogInputAcquisitionDuration();
	if (!(std::fabs(s - actual) < std::numeric_limits<double>::epsilon())) {
		std::string dbg = "Acquisition Duration Differs: desired=" + std::to_string(s) +
		" actual=" + std::to_string(actual);
		debug(Device::DebugLevelDebug, dbg);
	}
	return actual;
}

double Device::analogInputAcquisitionDuration()
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	double s;
	checkAndThrow(FDwfAnalogInRecordLengthGet(m_devHandle, &s),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

	return s;
}

void Device::setAnalogInputReconfigure(bool r)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInConfigure(m_devHandle, r, false),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

}

void Device::setAnalogInputStart(bool s)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInConfigure(m_devHandle, false, s),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

void Device::setAnalogInputBufferSize(int s)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInBufferSizeSet(m_devHandle, s),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

void Device::setAnalogInputTriggerSource(TriggerSource t)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInTriggerSourceSet(m_devHandle, static_cast<unsigned char>(t)),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

void Device::setAnalogInputTriggerAutoTimeout(double t)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInTriggerAutoTimeoutSet(m_devHandle, t),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

void Device::setAnalogInputTriggerChannel(int c)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInTriggerChannelSet(m_devHandle, c),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

void Device::setAnalogInputTriggerType(TriggerType t)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInTriggerTypeSet(m_devHandle, static_cast<int>(t)),
			__PRETTY_FUNCTION__, __FILE__, __LINE__);
}

void Device::setAnalogInputTriggerLevel(double l)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInTriggerLevelSet(m_devHandle, l),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

void Device::setAnalogInputTriggerCondition(TriggerCondition t)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInTriggerConditionSet(m_devHandle, static_cast<int>(t)),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

void Device::triggerAnalogInput()
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfDeviceTriggerPC(m_devHandle),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

int Device::analogInputBufferSize()
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);
	int size;
	checkAndThrow(FDwfAnalogInBufferSizeInfo(m_devHandle, nullptr, &size),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
	return size;
}

Device::SampleState Device::analogInSampleState()
{
	DwfState state = 0;
	checkAndThrow(FDwfAnalogInStatus(m_devHandle, true, &state),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

	Device::SampleState ret = {-1, -1, -1};

	checkAndThrow(FDwfAnalogInStatusRecord(m_devHandle, &ret.available, &ret.lost, &ret.corrupted),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

	return ret;
}

void Device::readAnalogInput(double *buffer, int size)
{
	checkAndThrow(FDwfAnalogInStatusData(m_devHandle, Device::s_channelId, buffer, size),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

//Static
std::list<Device::DeviceId> Device::getDevices()
{
	int devCount;
	FDwfEnum(enumfilterAll, &devCount);

	std::list<Device::DeviceId> ret;
	for (Device::DeviceId d = {0,0,0}; d.index<devCount; d.index++) {
		FDwfEnumDeviceType(d.index, &d.id, &d.ver);
		ret.push_back(d);
	}
	return ret;
}

//Static
int Device::channelId()
{
	return Device::s_channelId;
}

//Static
Device::DebugLevel Device::debugLevel()
{
	return s_debugLevel;
}

//Static
void Device::debug(DebugLevel level, const std::string& msg)
{
	if (debugLevel() >= level)
		std::cout << "[" << Device::s_debugLevelNames[level] << "]: " << msg << std::endl;
}

//Static
void Device::readSamples(Device *handle, double *buffer, int bufferSize, std::vector<double> *target, int available)
{
	while (available) {
		int count = available > bufferSize ? bufferSize : available;
		handle->readAnalogInput(buffer, count);
		copy(&buffer[0], &buffer[count], back_inserter(*target));
		available -= count;
	}

}

SharedTerminateFlag createTerminateFlag()
{
	return SharedTerminateFlag(new std::atomic<bool>(false));
}

std::ostream& operator<<(std::ostream& lhs, const Device::DeviceState& rhs)
{
	return lhs << Device::s_stateNames[rhs];
}

std::ostream& operator<<(std::ostream& lhs, const Device::SampleState& rhs)
{
	return lhs << "available=" << rhs.available
			   << " lost=" << rhs.lost
			   << " corrupted=" << rhs.corrupted;
}
