#include <iostream>
#include <thread>

#include "measurement.h"
#include "analogdiscovery.h"
#include "gpio.h"

using namespace std;




int main(int argc, char *argv[])
{
	auto devs = AnalogDiscovery::getDevices();
	if (devs.empty()) {
		std::cout << "No Analog Discovery devices Found!" << std::endl;
		exit(0);
	}
	auto sharedDev = createSharedAnalogDiscoveryHandle(devs.front());


	auto gpios = loadDefaultGPIOMapping(sharedDev);

	auto gpioSnapshotWoofer = createGPIOSnapshot(gpios);
	updateGPIOSnapshot(&gpioSnapshotWoofer, getGPIOForName(gpios, "Relais_K109"), {GPIO::DirectionOut, true});
	updateGPIOSnapshot(&gpioSnapshotWoofer, getGPIOForName(gpios, "Relais_K104"), {GPIO::DirectionOut, true});
	updateGPIOSnapshot(&gpioSnapshotWoofer, getGPIOForName(gpios, "Relais_K108"), {GPIO::DirectionOut, true});





	return 0;


}
