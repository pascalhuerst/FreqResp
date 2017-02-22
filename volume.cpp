#include "volume.h"

#include <thread>

using namespace std::chrono_literals;

Volume::Volume()
{}

Volume::~Volume()
{}

VolumeButtons::VolumeButtons(SharedGPIOHandle up, SharedGPIOHandle down) :
	basetype(),
	m_gpioUp(up),
	m_gpioDown(down)
{}

VolumeButtons::~VolumeButtons()
{}

void VolumeButtons::up()
{
	//TODO: Don't want to delay here. Check if this is too fast
	// One edge -> volume +1
	std::cout << "buttons up" << std::endl;
	m_gpioUp->setValue(!m_gpioUp->getValue());
	std::this_thread::sleep_for(10ms);
	m_gpioUp->setValue(!m_gpioUp->getValue());
	std::this_thread::sleep_for(10ms);
}

void VolumeButtons::down()
{
	//TODO: Don't want to delay here. Check if this is too fast
	// One edge -> volume -1
	std::cout << "buttons down" << std::endl;
	m_gpioDown->setValue(!m_gpioDown->getValue());
	std::this_thread::sleep_for(10ms);
	m_gpioDown->setValue(!m_gpioDown->getValue());
	std::this_thread::sleep_for(10ms);
}

VolumeEncoder::VolumeEncoder(SharedGPIOHandle a, SharedGPIOHandle b) :
	basetype(),
	m_encoder(a, b)
{}

VolumeEncoder::~VolumeEncoder()
{}

void VolumeEncoder::up()
{
	std::cout << "encoder up" << std::endl;
	m_encoder.increment();
}

void VolumeEncoder::down()
{
	std::cout << "encoder down" << std::endl;
	m_encoder.decrement();
}

SharedVolumeHandle createVolume(SharedGPIOHandle a, SharedGPIOHandle b, bool isEncoder)
{
	if (isEncoder)
		return SharedVolumeHandle(new VolumeEncoder(a,b));
	else
		return SharedVolumeHandle(new VolumeButtons(a,b));
}
