#include "gpio.h"
#include "analogdiscovery.h"

#include <fstream>

GPIOException::GPIOException(std::string func, std::string file, int line, int errorNumber, std::string what) :
	m_func(func),
	m_file(file),
	m_msg(what),
	m_line(line),
	m_errno(errorNumber)
{}

const char* GPIOException::what() const noexcept
{
	return ("GPIOException caught:\n  " + m_file + ":" + std::to_string(m_line) + "\n  " + m_func  + "\n  errno=" + std::to_string(m_errno) + "\n  " + m_msg).c_str();
}

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
{
	if (!m_sharedAdHandle)
		throw GPIOException(__PRETTY_FUNCTION__, __FILE__, __LINE__, 0,
							"AnalogDiscovery handle is nullptr");
}

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
	std::string fileExportPath = s_basePath + "/export";
	fileExport.open(fileExportPath);
	if (!fileExport.is_open())
		throw GPIOException(__PRETTY_FUNCTION__, __FILE__, __LINE__, 0,
							"Can not open file: " + fileExportPath + " to export GPIO" + std::to_string(m_gpioNumber));

	fileExport << m_gpioNumber << std::endl;
	fileExport.flush();
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
	std::string fileUnexportPath = s_basePath + "/unexport";
	fileUnexport.open(fileUnexportPath);
	fileUnexport << m_gpioNumber << std::endl;
	fileUnexport.flush();
}

void GPIOSysFs::setDirection(Direction d)
{
	std::ofstream fileDirection;
	std::string fileDirectionPath = s_basePath + "/gpio" + std::to_string(m_gpioNumber) + "/direction";
	fileDirection.open(fileDirectionPath);
	if (!fileDirection.is_open())
		throw GPIOException(__PRETTY_FUNCTION__, __FILE__, __LINE__, 0,
							"Can not open file: " + fileDirectionPath + " to set direction for GPIO" + std::to_string(m_gpioNumber));

	fileDirection << (d == DirectionIn ? "in" : "out") << std::endl;
	fileDirection.flush();
}

void GPIOSysFs::setValue(bool v)
{
	std::ofstream fileValue;
	std::string fileValuePath = s_basePath + "/gpio" + std::to_string(m_gpioNumber) + "/value";
	fileValue.open(fileValuePath);
	if (!fileValue.is_open())
		throw GPIOException(__PRETTY_FUNCTION__, __FILE__, __LINE__, 0,
							"Can not open file: " + fileValuePath + " to set value for GPIO" + std::to_string(m_gpioNumber));

	fileValue << (v ? "1" : "0") << std::endl;
	fileValue.flush();
}

GPIO::Direction GPIOSysFs::getDirection() const
{
	std::ifstream fileDirection;
	std::string fileDirectionPath = s_basePath + "/gpio" + std::to_string(m_gpioNumber) + "/direction";
	fileDirection.open(fileDirectionPath);
	if (!fileDirection.is_open())
		throw GPIOException(__PRETTY_FUNCTION__, __FILE__, __LINE__, 0,
							"Can not open file: " + fileDirectionPath + " to get direction for GPIO" + std::to_string(m_gpioNumber));

	std::string line;
	std::getline(fileDirection, line);

	if (line != "in" && line != "out")
		throw GPIOException(__PRETTY_FUNCTION__, __FILE__, __LINE__, 0,
							"Invalid value (" + line + ") while reading direction for GPIO" + std::to_string(m_gpioNumber));

	return (line == "in" ? DirectionIn : DirectionOut);
}

bool GPIOSysFs::getValue() const
{
	std::ifstream fileValue;
	std::string fileValuePath = s_basePath + "/gpio" + std::to_string(m_gpioNumber) + "/value";
	fileValue.open(fileValuePath);
	if (!fileValue.is_open())
		throw GPIOException(__PRETTY_FUNCTION__, __FILE__, __LINE__, 0,
							"Can not open file: " + fileValuePath + " to get value for GPIO" + std::to_string(m_gpioNumber));

	std::string line;
	std::getline(fileValue, line);

	if (line != "1" && line != "0")
		throw GPIOException(__PRETTY_FUNCTION__, __FILE__, __LINE__, 0,
							"Invalid value (" + line + ") while reading dvaliue for GPIO" + std::to_string(m_gpioNumber));

	return (line == "1" ? true : false);
}

SharedGPIOHandle createGPIO(const std::string &name, int gpioNumber, GPIO::Direction d, bool value)
{
	return std::unique_ptr<GPIO>(new GPIOSysFs(name, gpioNumber, d, value));
}

// This is a bit hackish, but works good to test-toggle GPIOs by hand.
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
