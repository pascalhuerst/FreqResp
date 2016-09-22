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
		FDwfDeviceClose(this->m_devHandle);
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

	//checkAndThrow(FDwfAnalogInRecordLengthSet(m_devHandle, samplingDurationS),
	//			  __PRETTY_FUNCTION__, __FILE__, __LINE__);
	//double acquiredDurationS;
	//checkAndThrow(FDwfAnalogInRecordLengthGet(m_devHandle, &acquiredDurationS),
	//			  __PRETTY_FUNCTION__, __FILE__, __LINE__);
	//std::cout << "samplingDuration=" << acquiredDurationS << std::endl;

	checkAndThrow(FDwfAnalogInConfigure(this->m_devHandle, false, true),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}

Device::SampleState Device::analogInSampleState()
{
	DwfState state = 0;
	checkAndThrow(FDwfAnalogInStatus(m_devHandle, true, &state),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);


	Device::SampleState ret = {-1, -1, -1};
	// These flags seem to be obsolete ?!
	if(state == stsCfg || state == stsPrefill || state == stsArm){
		return ret;
	}

	checkAndThrow(FDwfAnalogInStatusRecord(m_devHandle, &ret.available, &ret.lost, &ret.corrupted),
				  __PRETTY_FUNCTION__, __FILE__, __LINE__);

	return ret;
}

void Device::readAnalogInput(double *buffer, int size)
{
		checkAndThrow(FDwfAnalogInStatusData(m_devHandle, Device::s_channelId, buffer, size),
					  __PRETTY_FUNCTION__, __FILE__, __LINE__);
}



bool Device::read_rms(double frequency, int num_samples, double& rms_out)
{
	if (!isOpen()) {
		std::cout << "Analog Discovery device not accessible" << std::endl << std::flush;
		return false;
	}

	// Configure input to take num_samples samples at 64 time higher than signal frequency
	setInputConfig(frequency * 32, num_samples);
	setOutputConfig(frequency);

	// Wait 1ms to settle output freq
	// This is just kept for the safety purpose, and can be removed while
	// doing actual measurements.
	//	usleep(1000);

	STS status;
	int captured_samples = 0;
	int available_samples, lost_samples, corrupted_samples;
	bool lost, corrupted;
	double* samples_data;
	int cs_sum = 0;

	// Allocate 512 more samples to accommodate last read
	samples_data = new double[num_samples + 512];
	if (samples_data == nullptr) {
		std::cout << "Memory allocation failed for freq: " << frequency << std::endl << std::flush;
		return false;
	}

	while (captured_samples < num_samples) {

		if(!FDwfAnalogInStatus(this->m_devHandle, true, &status)) {
			return false;
		}

		if(captured_samples == 0 && (status == stsCfg || status == stsPrefill || status == stsArm)){
			// Acquisition not yet started.
			continue;
		}

		FDwfAnalogInStatusRecord(this->m_devHandle, &available_samples,	&lost_samples, &corrupted_samples);

		available_samples += lost_samples;

		if(lost_samples != 0) lost = true;
		if(corrupted_samples != 0) {
			corrupted = true;
			cs_sum += corrupted_samples;
		}

		if(!available_samples) continue;

		// by the time last call to read status is made, the
		// captured_samples can go beyond num_samples if
		// available_samples are added to it.
		FDwfAnalogInStatusData(this->m_devHandle, 0, &samples_data[captured_samples], available_samples);
		captured_samples += available_samples;

	}

	rms_out = 0;
	for (int i = 0; i < captured_samples; i++)
		rms_out += pow(samples_data[i], 2);

	rms_out = sqrt(rms_out / captured_samples);
	if (cs_sum) {
		std::cout << "Corrupted Samples: " << cs_sum << std::endl;
		std::cout << "Freq: " << frequency << "\tRMS: " << rms_out << std::endl << std::flush;
	}
	std::cout << "Freq: " << frequency << "\tRMS: " << rms_out << std::endl << std::flush;

	delete[] samples_data;

	return true;
}

SharedTerminateFlag createTerminateFlag()
{
	return SharedTerminateFlag(new std::atomic<bool>(false));
}
