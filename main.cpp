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
using namespace std::chrono_literals;

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

				(paramOutputFile, value<std::string>(), "Save data to file")

				(paramSelfTest, "Run selftest to verify hw integrity")
				(paramManualGpio, "Run manual GPIO test application")
				(paramCalibrate, "Run input level calibration")

				(paramListGpios, "List available GPIOs")
				(paramSetGpios, boost::program_options::value<std::vector<std::string>>()->multitoken(),  "arg=(name,value name,value ...) Set GPIO(s) to value")

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
		if (varMap.count(paramHelp)) { printUsage(desc, ""); }

		if (varMap.count(paramDebugLevel)) {
			Debug::setDebugLevel(static_cast<Debug::Level>(varMap["debug"].as<int>()));
		}

		// Testing and single functions
		if (varMap.count(paramSelfTest)) {
			std::cout << "Running selftest" << std::endl;
			exit(EXIT_SUCCESS);
		}

		if (varMap.count(paramManualGpio)) {
			{ manualGPIOTest(); }
			exit(EXIT_SUCCESS);
		}

		if (varMap.count(paramCalibrate)) {
			manualInputLevelCalibration();
			exit(EXIT_SUCCESS);
		}

		// GPIO
		if (varMap.count(paramListGpios)) {
			{ // exit() kills RAII, so make an extra block here
				auto sharedDev = AnalogDiscovery::getFirstAvailableDevice();
				auto gpios = loadDefaultGPIOMapping(sharedDev);
				for (auto iter = gpios.begin(); iter != gpios.end(); ++iter)
					std::cout << (*iter)->getName() << std::endl;
			}
			exit(EXIT_SUCCESS);
		}

		if (varMap.count(paramSetGpios)) {
			Debug::warning("set-gpios", "Analog Discovery gpio changes are reset, after the interface is closed for some time. Therefore this tool keeps running, to hold the gpio to what you set it.");
			auto v = varMap[paramSetGpios].as<vector<std::string>>();

			for (auto iter=v.begin(); iter!=v.end(); ++iter) {
				std::istringstream ss(*iter);
				std::string gpioName;
				std::string gpioValue;

				std::getline(ss, gpioName, ',');
				std::getline(ss, gpioValue, ',');
				std::transform(gpioValue.begin(), gpioValue.end(), gpioValue.begin(),
							   [](unsigned char c) { return std::tolower(c);});

				int gpioValueToSet;
				if (gpioValue == "hi" || gpioValue == "1") {
					gpioValueToSet = 1;
				} else if (gpioValue == "lo" || gpioValue == "0") {
					gpioValueToSet = 0;
				} else {
					Debug::error(paramSetGpios, "Invalid value for gpio. Must be hi,1 or lo,0");
					exit(EXIT_FAILURE);
				}

				auto sharedDev = AnalogDiscovery::getFirstAvailableDevice();
				auto gpios = loadDefaultGPIOMapping(sharedDev);
				auto gpio = getGPIOForName(gpios, gpioName);
				if (!gpio) {
					Debug::error(paramSetGpios, "GPIO with name " + gpioName + " seems not to exist!");
					exit(EXIT_FAILURE);
				} else {
					Debug::warning(paramSetGpios, "GPIO Ready: " + gpio->getName());
				}

				//gpio->setDirection(GPIO::DirectionOut);
				gpio->setValue(gpioValueToSet);
			}

			getchar();

			exit(EXIT_SUCCESS);
		}


		// Measuring
		if (varMap.count(paramChannel)) {
			channel = varMap[paramChannel].as<char>();
			if (channel != 'l' && channel != 'r')
				printUsage(desc, "invalid value for channel");
		} else {
			printUsage(desc, "channel must be set!");
		}

		if (varMap.count(paramSpeakerChannel)) {
			auto sp = varMap[paramSpeakerChannel].as<std::string>();

			if (sp == "lo") speakerChannel = Speaker::Lo;
			else if (sp == "mid") speakerChannel = Speaker::Mid;
			else if (sp == "hi") speakerChannel = Speaker::Hi;
			else printUsage(desc, "Invalid value for speakerchannel");
		} else {
			printUsage(desc, "speakerchannel must be set!");
		}

		if (varMap.count(paramOutputFile)) {
			outputName = varMap[paramOutputFile].as<std::string>();
		}

		if (varMap.count(paramfMin)) {
			fMin = varMap[paramfMin].as<double>();
		}

		if (varMap.count(paramfMax)) {
			fMax = varMap[paramfMax].as<double>();
		}

		if (varMap.count(paramPointsPerDecade)) {
			pointsPerDecade = varMap[paramPointsPerDecade].as<int>();
		}

		if (varMap.count(paramOutputCalibration)) {
			outputCalibration = varMap[paramOutputCalibration].as<double>();
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
		std::thread t1(mapInToOut, getGPIOForName(gpios, "Led 1"), getGPIOForName(gpios, "Front_LED_1"), 20ms, tf1);
		SharedTerminateFlag tf2 = SharedTerminateFlag(new std::atomic<bool>(false));
		std::thread t2(mapInToOut, getGPIOForName(gpios, "Led 2"), getGPIOForName(gpios, "Front_LED_2"), 20ms, tf2);


		Measurement m(outputName, sharedDev, fMin, fMax, pointsPerDecade);

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

	} catch (const AnalogDiscoveryException &e) {
		cerr << "AnalogDiscoveryException caught: " << e.what() << std::endl;
		cerr << where(e);
		exit(EXIT_FAILURE);
	} catch (const GPIOException &e) {
		cerr << "GPIOException caught: " << e.what() << std::endl;
		cerr << where(e);
		exit(EXIT_FAILURE);
	} catch (const exception &e) {
		cout << "std::exception caught: " << e.what() << std::endl;
		exit(EXIT_FAILURE);
	} catch (...) {
		cerr << "Unknown exception caught! Abort." << std::endl;
	}

	return 0;

}
