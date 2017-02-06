#include "gpio.h"
#include "analogdiscovery.h"

#include <fstream>

GPIO::GPIO(const std::string &name) :
	m_name(name)
{
}

GPIO::~GPIO()
{}

std::string GPIO::getName() const
{
	return m_name;
}

// Analog Discovery

GPIOAnalogDiscovery::GPIOAnalogDiscovery(const std::string &name, std::shared_ptr<AnalogDiscovery> ad, unsigned int gpioNumber) :
	basetype(name),
	m_sharedAdHandle(ad),
	m_gpioNumber(gpioNumber)
{}

GPIOAnalogDiscovery::GPIOAnalogDiscovery(const std::string &name, std::shared_ptr<AnalogDiscovery> ad, unsigned int gpioNumber, Direction d, bool value) :
	GPIOAnalogDiscovery(name, ad, gpioNumber)
{
	setDirection(d);
	setValue(value);
}

GPIOAnalogDiscovery::~GPIOAnalogDiscovery()
{}

void GPIOAnalogDiscovery::setDirection(Direction d)
{
	m_sharedAdHandle->setDigitalIoDirection(m_gpioNumber, static_cast<AnalogDiscovery::IODirection>(d));
}

void GPIOAnalogDiscovery::setValue(bool v)
{
	m_sharedAdHandle->setDigitalIo(m_gpioNumber, v);
}

GPIO::Direction GPIOAnalogDiscovery::getDirection() const
{
	return static_cast<GPIO::Direction>(m_sharedAdHandle->getDigitalIoDirection(m_gpioNumber));
}

bool GPIOAnalogDiscovery::getValue() const
{
	return m_sharedAdHandle->getDigitalIo(m_gpioNumber);
}

SharedGPIOHandle createGPIO(const std::string &name, std::shared_ptr<AnalogDiscovery> ad, unsigned int gpioNumber, GPIO::Direction d, bool value)
{
	return  std::unique_ptr<GPIO>(new GPIOAnalogDiscovery(name, ad, gpioNumber, d, value));
}

// SYS FS
const std::string  GPIOSysFs::s_basePath = "/sys/class/gpio";


GPIOSysFs::GPIOSysFs(const std::string &name, int gpioNumber) :
	basetype(name),
	m_gpioNumber(gpioNumber)
{
	std::ofstream fileExport;
	fileExport.open(s_basePath + "/export");
	fileExport << m_gpioNumber << std::endl;
}

GPIOSysFs::GPIOSysFs(const std::string &name, int gpioNumber, Direction d, bool value) :
	GPIOSysFs(name, gpioNumber)
{
	setDirection(d);
	setValue(value);
}

GPIOSysFs::~GPIOSysFs()
{
	std::ofstream fileUnexport;
	fileUnexport.open(s_basePath + "/unexport");
	fileUnexport << m_gpioNumber << std::endl;
	fileUnexport.flush();
}

void GPIOSysFs::setDirection(Direction d)
{
	std::ofstream fileDirection;
	fileDirection.open(s_basePath + "/gpio" + std::to_string(m_gpioNumber) + "/direction");
	fileDirection << (d == DirectionIn ? "in" : "out") << std::endl;
	fileDirection.flush();
}

void GPIOSysFs::setValue(bool v)
{
	std::ofstream fileValue;
	fileValue.open(s_basePath + "/gpio" + std::to_string(m_gpioNumber) + "/value");

	if (!fileValue.is_open()) std::cout << "value FILE NOT OPEN" << std::endl;

	fileValue << (v ? "1" : "0") << std::endl;
	fileValue.flush();
}

GPIO::Direction GPIOSysFs::getDirection() const
{
	std::ifstream fileDirection;
	fileDirection.open(s_basePath + "/gpio" + std::to_string(m_gpioNumber) + "/direction");
	std::string line;
	std::getline(fileDirection, line);

	//TODO: Better errorhandling
	return (line == "in" ? DirectionIn : DirectionOut);
}

bool GPIOSysFs::getValue() const
{
	std::ifstream fileValue;
	fileValue.open(s_basePath + "/gpio" + std::to_string(m_gpioNumber) + "/value");
	std::string line;
	std::getline(fileValue, line);

	return (line == "1" ? true : false);
}

SharedGPIOHandle createGPIO(const std::string &name, int gpioNumber, GPIO::Direction d, bool value)
{
	return std::unique_ptr<GPIO>(new GPIOSysFs(name, gpioNumber, d, value));
}

// This is a bit hackish, but works good to toggle GPIOs by hand.
void manualTest(std::list<SharedGPIOHandle> gpios)
{
	char g = 0;

	do {
		if (g) {
			int distance = g - 'A';
			if (distance < gpios.size()) {
				auto iter = gpios.begin();
				while(distance--) iter++;
				(*iter)->setValue(!(*iter)->getValue());
			}
		}

		char v = 'A';
		std::cout << "### Use keys on the left to toggle output, 'q' for exit: ###" << std::endl << std::endl;
		for (auto iter = gpios.begin(); iter != gpios.end(); ++iter, v++)
			std::cout << v << "      " << (*iter)->getValue() << "  "
					  << ((*iter)->getDirection() == GPIO::DirectionIn ? "IN   " : "OUT  ")
					  << (*iter)->getName() << std::endl;

		std::cout << std::endl << std::endl;

		g = getchar();

	} while(g != 'q');

}
