#include <iostream>
#include <thread>
#include <fstream>
#include <iterator>

#include "Device.h"

using namespace std;

void saveBuffer(SharedSampleStorage s)
{
	std::ofstream outfile("/home/paso/development/raumfeld/digilent/build-FreqResp-Qt_5_5_0_gcc_64-Default/samples.txt", std::ofstream::out);

	if (!outfile.is_open()) {
		std::cout << "File not opened!" << std::endl;
	}

	Indexer<double>::setStartIndex(0);
	std::ostream_iterator<Indexer<double>> iter(outfile, "\n");
	std::copy(s->end()-2000, s->end(), iter);
	outfile.flush();
	outfile.close();
}


int main(int argc, char *argv[])
{

	auto devs = Device::getDevices();
	if (devs.empty()) {
		std::cout << "No devices Found!" << std::endl;
		exit(0);
	}

	std::cout << "Found " << devs.size() << " devices. Opening " <<
				 devs.front().id << " [" << devs.front().ver << "]" << std::endl;

	try {
		Device dev(devs.front());

		// Todo: This function names don't make sense to me. Order matters, but
		// doesn't make sense ?!
		dev.enableOutput(2);

		//double freq = 50;
		//dev.setOutputConfig(freq);

		dev.setInputConfig(40000, -1);
		dev.startAcquisition(10);


		auto terminate = createTerminateFlag();
		SharedSampleStorage samples(new std::vector<double>());
		std::thread t1(readSamplesFunction, &dev, samples, terminate);

		std::cout << "Press any key for status or q to exit..." << std::endl;
		while (getchar() != 'q') {

			//freq += 1000.f;
			//dev.setOutputConfig(freq);

			std::cout << "InputStatus:  " << dev.inputStatus() << std::endl;
			std::cout << "OutputStatus: " << dev.outputStatus() << std::endl;
			std::cout << "Samples:      " << samples->size() << std::endl;

		}

		terminate->store(true);
		t1.join();

		saveBuffer(samples);

	} catch (DeviceException e) {
		std::cout << e.what();
	}



	return 0;
}
