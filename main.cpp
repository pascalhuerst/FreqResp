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

void printUsage(const options_description &desc, const std::string& extraMsg)
{
	cout << desc << std::endl << extraMsg << std::endl;
	exit(EXIT_FAILURE);
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

				(paramSpeakerChannel, value<std::string>(), "arg=(lo,mid,hi) Set speaker output channel to measure on")
				(paramChannel, value<char>(), "arg=(l|r) Set channel you want to measure on")
				(paramfMin, value<double>(), "arg=f [Hz] Set lower frequency to start frequency response measurement with")
				(paramfMax, value<double>(), "arg=f [Hz] Set upper frequency to stop frequency response measurement with")
				(paramPointsPerDecade, value<int>(), "arg=p [Points per decade] Set amount of measuring points per decade")
				(paramOutputCalibration, value<double>(), "arg=v [V] Adjust output value of sine sweep by this value. Use --calibrate to find that value");

		variables_map varMap;
		store(parse_command_line(argc, argv, desc), varMap);
		notify(varMap);

		// Information and Debug
		if (varMap.count("help")) { printUsage(desc, ""); }

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

		// Measuring
		if (varMap.count("channel")) {
			channel = varMap["channel"].as<char>();
			if (channel != 'l' && channel != 'r')
				printUsage(desc, "invalid value for channel");
		} else {
			printUsage(desc, "channel must be set!");
		}

		if (varMap.count("speakerchannel")) {
			auto sp = varMap["speakerchannel"].as<std::string>();

			if (sp == "lo") speakerChannel = Speaker::Lo;
			else if (sp == "mid") speakerChannel = Speaker::Mid;
			else if (sp == "hi") speakerChannel = Speaker::Hi;
			else printUsage(desc, "Invalid value for speakerchannel");
		} else {
			printUsage(desc, "speakerchannel must be set!");
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

		if (varMap.count("output-calibration")) {
			outputCalibration = varMap["output-calibration"].as<double>();
		}


		auto sharedDev = AnalogDiscovery::getFirstAvailableDevice();
		auto gpios = loadDefaultGPIOMapping(sharedDev);

		auto adr0 = getGPIOForName(gpios, "ADR0");
		auto adr1 = getGPIOForName(gpios, "ADR1");
		auto enable = getGPIOForName(gpios, "Enable");
		auto speakerPower = getGPIOForName(gpios, "Relais_Power");
		speakerPower->setValue(true);

		Speaker::setChannel(enable, adr0, adr1, speakerChannel);

		// map Speaker Led {1,2} to front Led {1,2}
		SharedTerminateFlag tf1 = SharedTerminateFlag(new std::atomic<bool>(false));
		std::thread t1(mapInToOut, getGPIOForName(gpios, "Led 1"), getGPIOForName(gpios, "Front_LED_1"), 50, tf1);
		SharedTerminateFlag tf2 = SharedTerminateFlag(new std::atomic<bool>(false));
		std::thread t2(mapInToOut, getGPIOForName(gpios, "Led 2"), getGPIOForName(gpios, "Front_LED_2"), 50, tf2);


		Measurement m("MyMeasurement", sharedDev, fMin, fMax, pointsPerDecade);

		std::cout << "Press enter to start..." << std::endl;
		getchar();

		// Fix me!
		m.start((channel == 'r' ? 0 : 1), outputCalibration);

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

	} catch (AnalogDiscoveryException& e) {
		cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	} catch (exception& e) {
		cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return 0;

}
