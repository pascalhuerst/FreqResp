#pragma once

#include <memory>
#include <string>
#include <fstream>
#include <list>

class GPIO
{
public:
	GPIO(const std::string &name);
	virtual ~GPIO();

	enum Direction {
		DirectionIn = 0x01,
		DirectionOut = 0x00
	};

	virtual void setDirection(Direction d) = 0;
	virtual void setValue(bool v) = 0;

	virtual Direction getDirection() const = 0;
	virtual bool getValue() const = 0;

	std::string getName() const;

private:
	std::string m_name;
};

typedef std::shared_ptr<GPIO> SharedGPIOHandle;

class AnalogDiscovery;

class GPIOAnalogDiscovery : public GPIO
{
public:
	typedef GPIO basetype;

	GPIOAnalogDiscovery(const std::string &name, std::shared_ptr<AnalogDiscovery> ad, unsigned int gpioNumber);
	GPIOAnalogDiscovery(const std::string &name, std::shared_ptr<AnalogDiscovery> ad, unsigned int gpioNumber, Direction d, bool value);
	virtual ~GPIOAnalogDiscovery();

	virtual void setDirection(Direction d);
	virtual void setValue(bool v);

	virtual Direction getDirection() const;
	virtual bool getValue() const;

private:
	std::shared_ptr<AnalogDiscovery> m_sharedAdHandle;
	unsigned int m_gpioNumber;
};

SharedGPIOHandle createGPIO(const std::string &name, std::shared_ptr<AnalogDiscovery> ad, unsigned int gpioNumber, GPIO::Direction d, bool value);

class GPIOSysFs : public GPIO
{
public:
	typedef GPIO basetype;

	GPIOSysFs(const std::string &name, int gpioNumber);
	GPIOSysFs(const std::string &name, int gpioNumber, Direction d, bool value);
	virtual ~GPIOSysFs();

	virtual void setDirection(Direction d);
	virtual void setValue(bool v);

	virtual Direction getDirection() const;
	virtual bool getValue() const;
private:
	int m_gpioNumber;
	const static std::string s_basePath;
};

SharedGPIOHandle createGPIO(const std::string &name, int gpioNumber, GPIO::Direction d, bool value);


void manualTest(std::list<SharedGPIOHandle> gpios);



