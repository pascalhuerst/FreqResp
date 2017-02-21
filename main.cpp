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

		std::string debugLevelDesc;
		for (int i = Debug::LevelNone; i < Debug::LevelLast; i++)
			debugLevelDesc += std::to_string(i) + " = " + Debug::name(static_cast<Debug::Level>(i)) + "\n";

		options_description desc("Usage");
		desc.add_options()
				(paramHelp, "print this message")
				(paramDebugLevel, value<int>(), debugLevelDesc.c_str())

				(paramSelfTest, "Run selftest to verify hw integrity")
				(paramManualGpio, "Run manual GPIO test application")
				(paramCalibrate, "Run input level calibration")

				(paramChannelId, value<int>(), "arg=(0|1) Set channel you want to measure on")
				(paramfMin, value<double>(), "arg=f [Hz] Set lower Frequency to start frequency response measurement with")
				(paramfMax, value<double>(), "arg=f [Hz] Set upper Frequency to stop frequency response measurement with")
				(paramPointsPerDecade, value<int>(), "arg=p [Points per decade] Set amount of measuring points per decade");


		variables_map varMap;
		store(parse_command_line(argc, argv, desc), varMap);
		notify(varMap);

		// Information and Debug
		if (varMap.count("help")) { printUsage(desc); exit(EXIT_SUCCESS); }

		if (varMap.count("debug")) {
			Debug::setDebugLevel(static_cast<Debug::Level>(varMap["debug"].as<int>()));
		}


		// Testing
		if (varMap.count("self-test")) {
			std::cout << "Running selftest" << std::endl;
			exit(EXIT_SUCCESS);
		}

		if (varMap.count("manual-gpio")) {
			manualGPIOTest();
			exit(EXIT_SUCCESS);
		}

		if (varMap.count("calibrate")) {
			manualInputLevelCalibration();
			exit(EXIT_SUCCESS);
		}

#if 0
		Debug::setDebugLevel(Debug::LevelVerbose);
		Debug::debug("DebugMessage", "This is an example message for this shot");
		Debug::error("ErrMessage", "This is an example message for this shot");
		Debug::verbose("Verbose Message", "This is an example message for this shot");
		Debug::warning("Warning Message", "This is an example message for this shot");

		return 0;
#endif

#if 1
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
#endif

		auto sharedDev = AnalogDiscovery::getFirstAvailableDevice();

#if 1
		auto gpios = loadDefaultGPIOMapping(sharedDev);
#else
		auto gpios = loadDummyGPIOMapping(sharedDev);
#endif

		auto gpioSnapshotDefault = createGPIOSnapshot(gpios);
		auto gpioSnapshotWoofer = gpioSnapshotDefault;

		// CHANNEL 0 / BASS current ADDR mapping
		if (!gpios.empty()) {

			// Set both speaker channels to 4Ohm
			updateGPIOSnapshot(&gpioSnapshotWoofer, getGPIOForName(gpios, "Relais_K104"), {GPIO::DirectionOut, true});
			updateGPIOSnapshot(&gpioSnapshotWoofer, getGPIOForName(gpios, "Relais_K108"), {GPIO::DirectionOut, true});
			// Select speaker channel 0 and 3
			updateGPIOSnapshot(&gpioSnapshotWoofer, getGPIOForName(gpios, "ADR0"), {GPIO::DirectionOut, false});
			updateGPIOSnapshot(&gpioSnapshotWoofer, getGPIOForName(gpios, "ADR1"), {GPIO::DirectionOut, true});
			// Enable speaker channel selection
			updateGPIOSnapshot(&gpioSnapshotWoofer, getGPIOForName(gpios, "Enable"), {GPIO::DirectionOut, true});
		}

		// map Speaker Led {1,2} to front Led {1,2}
		SharedTerminateFlag tf1 = SharedTerminateFlag(new std::atomic<bool>(false));
		std::thread t1(mapInToOut, getGPIOForName(gpios, "Led 1"), getGPIOForName(gpios, "Front_LED_1"), 500, tf1);
		SharedTerminateFlag tf2 = SharedTerminateFlag(new std::atomic<bool>(false));
		std::thread t2(mapInToOut, getGPIOForName(gpios, "Led 2"), getGPIOForName(gpios, "Front_LED_2"), 500, tf2);


		Measurement m("MyMeasurement", sharedDev, gpioSnapshotWoofer, fMin, fMax, pointsPerDecade);

		std::cout << "Press enter to start..." << std::endl;
		getchar();

		m.start(channelId);


		{ SpecialKeyboard kb; // nonblocking keyboard input
			while(kb.kbhit() != 'q' && m.isRunning()) {
				std::this_thread::sleep_for(std::chrono::milliseconds(250));
			}}

		if (m.isRunning()) m.stop();

		// Clean up LED Mapping threads
		tf1->store(true);
		tf2->store(true);
		t1.join();
		t2.join();

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
