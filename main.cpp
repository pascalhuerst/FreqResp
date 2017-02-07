#include <iostream>
#include <thread>

#include <unistd.h>
#include <termios.h>

#include "measurement.h"
#include "analogdiscovery.h"
#include "gpio.h"

#include <boost/program_options.hpp>

using namespace std;
using namespace boost::program_options;

void printUsage(const options_description &desc)
{
	cout << desc << std::endl;
}

char getch() {
		char buf = 0;
		struct termios old = {0};
		if (tcgetattr(0, &old) < 0)
				perror("tcsetattr()");
		old.c_lflag &= ~ICANON;
		old.c_lflag &= ~ECHO;
		old.c_cc[VMIN] = 1;
		old.c_cc[VTIME] = 0;
		if (tcsetattr(0, TCSANOW, &old) < 0)
				perror("tcsetattr ICANON");
		if (read(0, &buf, 1) < 0)
				perror ("read()");
		old.c_lflag |= ICANON;
		old.c_lflag |= ECHO;
		if (tcsetattr(0, TCSADRAIN, &old) < 0)
				perror ("tcsetattr ~ICANON");
		return (buf);
}



int main(int argc, char *argv[])
{
	// Command Line Parameters
	const char paramHelp[] = "help,h";
	const char paramSelfTest[] = "self-test,t";
	const char paramManualGpio[] = "manual-gpio,m";

	const char paramChannelId[] = "channel,c";
	const char paramfMin[] = "fmin,f";
	const char paramfMax[] = "fmax,g";
	const char paramPointsPerDecade[] = "points-per-decade,p";


	int channelId = -1;
	double fMin = 20;			// Default is 20Hz
	double fMax = 20000;		// Default is 20kHz
	int pointsPerDecade = 20;// 20 measurements per decade

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

		if (varMap.count("self-test")) {
			std::cout << "Running selftest" << std::endl;
			exit(EXIT_SUCCESS);
		}

		if (varMap.count("channel")) {
			channelId = varMap["channel"].as<int>();
			if (channelId != 0 && channelId != 1) {
				printUsage(desc);
				std::cout << "invalid value for channel" << std::endl;
				exit (EXIT_SUCCESS);
			}
		} else {
			printUsage(desc);
			std::cout << "channel must be set!" << std::endl;
			exit (EXIT_SUCCESS);
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



		if (varMap.count("manual-gpio")) {
			{ // Make sure RAII ressources are freed before exit()
				auto sharedDev = getFirstAvailableDevice();
				auto gpios = loadDefaultGPIOMapping(sharedDev);
				manualTest(gpios);
			}
			exit(EXIT_SUCCESS);
		}







		auto sharedDev = getFirstAvailableDevice();

#if 1
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


		//printf("%f %f\n", fMin, fMax);
		Measurement m("MyMeasurement", sharedDev, gpioSnapshotWoofer, fMin, fMax, pointsPerDecade);

		std::cout << "starting..." << std::endl;
		m.start(channelId);


		std::cout << "running..." << std::endl;
		while(getch() != 'q' && m.isRunning()) {
			std::cout << "...";
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}

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
