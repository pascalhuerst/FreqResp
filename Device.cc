#include "Device.h"
#include <iostream>
#include <math.h>
#include <unistd.h>

#include <chrono>
#include <thread>

using namespace std;

const int Device::s_channelId = 0;
const int Device::s_chunkSize = 1024;

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

DeviceException::DeviceException(std::string func, std::string file, int line, int errorNumber, std::string what) :
	m_func(func),
	m_file(file),
	m_line(line),
	m_errno(errorNumber),
	m_msg(what)
{}

const char* DeviceException::what() const noexcept
{
	return (m_file + ":" + m_func + ":" + std::to_string(m_line) + "(" + std::to_string(m_errno) + ")" + "\n   " + m_msg).c_str();
}

//static
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

//static
int Device::chunkSize()
{
	return Device::s_chunkSize;
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
}

Device::~Device(void)
{
	if (isOpen())
		FDwfDeviceClose(m_devHandle);
}

bool Device::isOpen(void) const
{
	return m_opened;
}

Device::DeviceState Device::outputStatus()
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

std::ostream& operator<<(std::ostream& lhs, const Device::DeviceState& rhs)
{
	return lhs << Device::s_stateNames[rhs];
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


void Device::setAnalogInputSamplingFreq(double f)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInFrequencySet(m_devHandle, f),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

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

void Device::setAnalogInputAcquisitionDuration(double s)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInRecordLengthSet(m_devHandle, s),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
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

int Device::analogInputGetBufferSize()
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
#if 0
	// These flags seem to be obsolete ?!
	if(state == stsCfg || state == stsPrefill || state == stsArm){
		return ret;
	}
#endif

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
int Device::channelId()
{
	return Device::s_channelId;
}

//Static
SharedTerminateFlag createTerminateFlag()
{
	return SharedTerminateFlag(new std::atomic<bool>(false));
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
