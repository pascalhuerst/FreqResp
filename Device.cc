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

Device::DeviceState Device::inputStatus()
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


/////////////////////////////
#if 0


void Device::startAcquisition(double voltRange)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	// Enable input channel s_channelId = 0
	checkAndThrow(FDwfAnalogInChannelEnableSet(m_devHandle, s_channelId, true),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInChannelRangeSet(m_devHandle, s_channelId, voltRange),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogInAcquisitionModeSet(m_devHandle, acqmodeRecord),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

	this_thread::sleep_for(chrono::milliseconds(2));

	// Start acquisition
	checkAndThrow(FDwfAnalogInConfigure(this->m_devHandle, false, true),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

void Device::enableOutput(double amplitude)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogOutNodeFunctionSet(this->m_devHandle, s_channelId, AnalogOutNodeCarrier, funcSine),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogOutNodeAmplitudeSet(this->m_devHandle, s_channelId, AnalogOutNodeCarrier, amplitude),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogOutNodeEnableSet(this->m_devHandle, s_channelId, AnalogOutNodeCarrier, true),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

void Device::setOutputConfig(double signalFreq)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogOutNodeFrequencySet(this->m_devHandle, s_channelId, AnalogOutNodeCarrier, signalFreq),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

	checkAndThrow(FDwfAnalogOutConfigure(this->m_devHandle, s_channelId, true),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

void Device::setInputConfig(double samplingFreq, double samplingDurationS)
{
	throwIfNotOpened(__PRETTY_FUNCTION__, __FILE__, __LINE__);

	// Set input sampling frequency
	checkAndThrow(FDwfAnalogInFrequencySet(m_devHandle, samplingFreq),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

	double acquiredFreq;
	checkAndThrow(FDwfAnalogInFrequencyGet(m_devHandle, &acquiredFreq),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
	std::cout << "samplingFreq=" << acquiredFreq << std::endl;
	checkAndThrow(FDwfAnalogInRecordLengthSet(m_devHandle, samplingDurationS),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
	double acquiredDurationS;
	checkAndThrow(FDwfAnalogInRecordLengthGet(m_devHandle, &acquiredDurationS),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
	std::cout << "samplingDuration=" << acquiredDurationS << std::endl;
	checkAndThrow(FDwfAnalogInConfigure(this->m_devHandle, false, true),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}
#endif


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
#if TEST == 1
	static double val = 0.f;
	for (int i=0; i<size; i++) {
		val += 0.1;
		buffer[i] = val;
	}
#else
	checkAndThrow(FDwfAnalogInStatusData(m_devHandle, Device::s_channelId, buffer, size),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
#endif
}

SharedTerminateFlag createTerminateFlag()
{
	return SharedTerminateFlag(new std::atomic<bool>(false));
}
