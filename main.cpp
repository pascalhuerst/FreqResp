#include <iostream>
#include <thread>

#include "measurement.h"
#include "analogdiscovery.h"
#include "gpio.h"
#include "default.h"
#include "specialkeyboard.h"
#include "tests.h"
#include "debug.h"

#include <boost/program_options.hpp>


using namespace std;
using namespace boost::program_options;

void printUsage(const options_description &desc)
{
	cout << desc << std::endl;
}


int main(int argc, char *argv[])
{
	try {

		options_description desc("Usage");
		desc.add_options()
				(paramHelp, "print this message")
				(paramSelfTest, "Run selftest to verify hw integrity")
				(paramManualGpio, "Run manual GPIO test application")

				(paramChannelId, value<int>(), "arg=(0|1) Set channel you want to measure on")
				(paramfMin, value<double>(), "arg=f [Hz] Set lower Frequency to start frequency response measurement with")
				(paramfMax, value<double>(), "arg=f [Hz] Set upper Frequency to stop frequency response measurement with")
				(paramPointsPerDecade, value<int>(), "arg=p [Points per decade] Set amount of measuring points per decade");


		variables_map varMap;
		store(parse_command_line(argc, argv, desc), varMap);
		notify(varMap);

		// Information
		if (varMap.count("help")) { printUsage(desc); exit(EXIT_SUCCESS); }

		// Testing
		if (varMap.count("self-test")) {
			std::cout << "Running selftest" << std::endl;
			exit(EXIT_SUCCESS);
		}

		if (varMap.count("manual-gpio")) {
			{ // Make sure RAII ressources are freed before exit()
				auto sharedDev = AnalogDiscovery::getFirstAvailableDevice();
				auto gpios = loadDefaultGPIOMapping(sharedDev);
				manualGPIOTest(gpios);
			}
			exit(EXIT_SUCCESS);
		}

		// Measuring
		if (varMap.count("channel")) {
			channelId = varMap["channel"].as<int>();
			if (channelId != 0 && channelId != 1) {
				printUsage(desc);
				std::cout << "invalid value for channel" << std::endl;
				exit (EXIT_FAILURE);
			}
		} else {
			printUsage(desc);
			std::cout << "channel must be set!" << std::endl;
			exit (EXIT_FAILURE);
		}

		if (varMap.count("fmin")) {
			fMin = varMap["fmin"].as<double>();
		}

		if (varMap.count("fmax")) {
			fMax = varMap["fmax"].as<double>();
		}

		if (varMap.count("points-per-decade")) {
			pointsPerDecade = varMap["points-per-decade"].as<int>();
		}


		auto sharedDev = AnalogDiscovery::getFirstAvailableDevice();

#if 0
		auto gpios = loadDefaultGPIOMapping(sharedDev);
#else
		auto gpios = loadDummyGPIOMapping(sharedDev);
#endif

		auto gpioSnapshotDefault = createGPIOSnapshot(gpios);
		auto gpioSnapshotWoofer = gpioSnapshotDefault;

		if (!gpios.empty()) {

			// Set both speaker channels to 4Ohm
			updateGPIOSnapshot(&gpioSnapshotWoofer, getGPIOForName(gpios, "Relais_K104"), {GPIO::DirectionOut, true});
			updateGPIOSnapshot(&gpioSnapshotWoofer, getGPIOForName(gpios, "Relais_K108"), {GPIO::DirectionOut, true});
			// Select speaker channel 0 and 3
			updateGPIOSnapshot(&gpioSnapshotWoofer, getGPIOForName(gpios, "ADR0"), {GPIO::DirectionOut, false}); //INVERTIERT KAPUTT
			updateGPIOSnapshot(&gpioSnapshotWoofer, getGPIOForName(gpios, "ADR1"), {GPIO::DirectionOut, false});
			// Enable speaker channel selection
			updateGPIOSnapshot(&gpioSnapshotWoofer, getGPIOForName(gpios, "nEnable"), {GPIO::DirectionOut, false});
		}



		Measurement m("MyMeasurement", sharedDev, gpioSnapshotWoofer, fMin, fMax, pointsPerDecade);
		m.start(channelId);

		{ SpecialKeyboard kb; // nonblocking keyboard input
			while(kb.kbhit() != 'q' && m.isRunning()) {
				std::cout << ".";
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}}

		if (m.isRunning()) m.stop();

		setGPIOSnapshot(gpioSnapshotDefault);

	} catch (AnalogDiscoveryException& e) {
		cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	} catch (exception& e) {
		cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return 0;

}
