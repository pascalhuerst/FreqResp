#include "tests.h"
#include "specialkeyboard.h"
#include "analogdiscovery.h"
#include "measurement.h"
#include "volume.h"
#include "speaker.h"

#include <iostream>
#include <thread>

using namespace std::chrono_literals;

// This is a bit hackish, but works good to test-toggle GPIOs by hand.
void manualGPIOTest()
{
	char g = 0;

	// nonblocking keyboard
	SpecialKeyboard kb;

	auto sharedDev = AnalogDiscovery::getFirstAvailableDevice();
	auto gpios = loadDefaultGPIOMapping(sharedDev);

	// map Speaker Led {1,2} to front Led {1,2}
	SharedTerminateFlag tf = createSharedTerminateFlag();
	std::thread t1(mapInToOut, getGPIOForName(gpios, "Led 1"), getGPIOForName(gpios, "Front_LED_1"), 10ms, tf);
	std::thread t2(mapInToOut, getGPIOForName(gpios, "Led 2"), getGPIOForName(gpios, "Front_LED_2"), 10ms, tf);

	do {

		unsigned int distance = g - 'A';
		if (distance < gpios.size()) {
			auto iter = gpios.begin();
			while(distance--) iter++;
			(*iter)->setValue(!(*iter)->getValue());
		}

		char v = 'A';
		std::cout << "### Use keys on the left to toggle output, 'q' for exit: ###" << std::endl << std::endl;
		for (auto iter = gpios.begin(); iter != gpios.end(); ++iter, v++)
			std::cout <<  v   << "      " << (*iter)->getValue() << "  "
					   << ((*iter)->getDirection() == GPIO::DirectionIn ? "IN   " : "OUT  ")
					   << (*iter)->getName() << std::endl;

		std::cout << std::endl << std::endl;

		while ((g = kb.kbhit()) == 0);
	} while (g != 'q');

	tf->store(true);
	t1.join();
	t2.join();
}

void manualInputLevelCalibration()
{
	char g = 0;

	// nonblocking keyboard input
	SpecialKeyboard kb;

	auto sharedDev = AnalogDiscovery::getFirstAvailableDevice();
	auto gpios = loadDefaultGPIOMapping(sharedDev);

	auto terminateRequest = createSharedTerminateFlag();

	auto adr0 = getGPIOForName(gpios, "ADR0");
	auto adr1 = getGPIOForName(gpios, "ADR1");
	auto enable = getGPIOForName(gpios, "Enable");
	auto speakerPower = getGPIOForName(gpios, "Relais_Power");
	speakerPower->setValue(true);


	auto calibrationAbout = createSharedCalibrateAmount();
	auto commandFlag = createSharedCommandFlag();
	auto volumeControl = createVolume(getGPIOForName(gpios, "Volume Button +"), getGPIOForName(gpios, "Volume Button -"), false);

	std::thread t1(Measurement::calibrate, terminateRequest, calibrationAbout, commandFlag, sharedDev);

	do {
		if (g == '+') {
			calibrationAbout->store(calibrationAbout->load() + 0.01);
		} else if (g == '-') {
			calibrationAbout->store(calibrationAbout->load() - 0.01);
		} else if (g == 'b') { // Tell thread function to save buffer as buffer.txt
			commandFlag->store('b');
		} else if (g == 'u') {
			volumeControl->up();
		} else if (g == 'd') {
			volumeControl->down();
		} else if (g == 'h') {
			Speaker::setChannel(enable, adr0, adr1, Speaker::Hi);
		} else if (g == 'm') {
			Speaker::setChannel(enable, adr0, adr1, Speaker::Mid);
		} else if (g == 'l') {
			Speaker::setChannel(enable, adr0, adr1, Speaker::Lo);
		}

		while ((g = kb.kbhit()) == 0) std::this_thread::sleep_for(std::chrono::milliseconds(100));
	} while(g != 'q');

	terminateRequest->store(true);
	t1.join();
}

void mapInToOut(SharedGPIOHandle in, SharedGPIOHandle out, std::chrono::milliseconds refreshRate, SharedTerminateFlag terminateRequest)
{
	while (!terminateRequest->load()) {
		out->setValue(in->getValue());
		std::this_thread::sleep_for(refreshRate);
	}
}
