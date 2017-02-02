#include <iostream>
#include <thread>
#include <fstream>
#include <iterator>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <list>

#include "analogdiscovery.h"
#include "gpio.h"

using namespace std;

double dBuForVolts(double v) {
	static const double Uref = 0.7746;
	return 20 * log(v / Uref);
}

double dBvForVolts(double v) {
	static const double Uref = 1.0;
	return 20 * log (v / Uref);
}

double rms(const std::vector<double>& samples)
{
	double rms = 0.0;
	for (auto sample : samples) {
		rms += sample * sample;
		rms = sqrt(rms / samples.size());
	}
	rms = sqrt(rms / samples.size());
	return rms;
}

// create logarithmically well distributed measuring points,
// so we have the same amount of measuring points in each decade.
void createMeasuringPoints(std::vector<double>& points, int pointsPerDecade, double minHz, double maxHz)
{
	// e Values per decade
	double e = static_cast<double>(pointsPerDecade);
	double k = std::pow(10, 1.0 / e);

	for (int exp=0; exp<e; exp++) {
		//double currentVal = round(pow(k, exp) * 10.0) / 10.0;
		double currentVal = (pow(k, exp) * 10.0) / 10.0;
		points.push_back(currentVal);

		double tmpMax = maxHz*10.0;
		double fac = 10.0;
		while (tmpMax > minHz) {
			points.push_back(currentVal*fac);
			tmpMax /= 10.0;
			fac *= 10.0;
		}
	}
	// Remove points within upper/lower decade to comply to minHz/maxHz;
	auto iter = std::remove_if(points.begin(), points.end(), [minHz,maxHz](double x){
		return x > maxHz || x < minHz;
	});

	points.erase(iter, points.end());
	std::sort(points.begin(), points.end());
}

void saveBuffer(const std::vector<double>& s, const std::string& fileName)
{
	std::ofstream outfile(fileName, std::ofstream::out);

	if (!outfile.is_open()) {
		std::cout << "File not opened!" << std::endl;
	}

	IntIndexer<double>::setStartIndex(0);
	std::ostream_iterator<IntIndexer<double>> iter(outfile, "\n");
	std::copy(s.begin(), s.end(), iter);
	outfile.flush();
	outfile.close();
}

void run(SharedTerminateFlag terminateRequest)
{
	auto devs = AnalogDiscovery::getDevices();
	if (devs.empty()) {
		std::cout << "No devices Found!" << std::endl;
		exit(0);
	}

	std::cout << "Found " << devs.size() << " devices. Opening " <<
				 devs.front().id << " [" << devs.front().ver << "]" << std::endl;

	AnalogDiscovery dev(devs.front());

	// Create frequency Map with measuring frequencies
	std::vector<double>points;
	double fMin = 20;
	double fMax = 20000;
	int pointsPerDecade = 100;
	createMeasuringPoints(points, pointsPerDecade, fMin, fMax);

	saveBuffer(points, "fMeasuringPoints.txt");

	std::vector<double>freqResp(points.size());

	try {

		const int channel = 0;
		dev.setAnalogOutputAmplitude(channel, 0.2); // 200mV
		dev.setAnalogOutputWaveform(channel, AnalogDiscovery::WaveformSine);

		auto currentFrequency = points.begin();
		auto currentFreqResp = freqResp.begin();

		while (!terminateRequest->load() && currentFrequency != points.end()) {

			dev.setAnalogOutputFrequency(channel, *currentFrequency);
			dev.setAnalogOutputEnabled(channel, true);
			std::this_thread::sleep_for(std::chrono::milliseconds(50));

			auto samples = readOneBuffer(&dev, *currentFrequency);

			// Remove upper and lower 10% leads to better results
			int removeCount = samples.size() * 0.1;
			samples.erase(samples.begin(), samples.begin()+removeCount);
			samples.erase(samples.end()-removeCount, samples.end());

			std::vector<double> keys(samples.size());
			std::iota(std::begin(keys), std::end(keys), 0);

			*currentFreqResp = dBuForVolts(rms(samples));

			std::cout << "[" << std::distance(points.begin(), currentFrequency)
					  << "] dBu @ " << double(*currentFrequency)
					  << "Hz: " << *currentFreqResp << std::endl;

			currentFrequency++;
			currentFreqResp++;

		}
	} catch(AnalogDiscoveryException e) {
		std::cout << "e.what()1: " << e.m_file + ":" + e.m_func + ":" + std::to_string(e.m_line) + "(" + std::to_string(e.m_errno) + ")" + "\n   " + e.m_msg << std::endl;
	} catch (std::exception e) {
		std::cout << "e.what()2: " << e.what() << std::endl;
	}
}

std::list<SharedGPIOHandle> loadGPIOMapping(std::shared_ptr<AnalogDiscovery> analogDiscovery)
{
	std::list<SharedGPIOHandle> gpios;

	// Analog Discovery GPIOs
	// Outputs
	gpios.push_back(createGPIO("Front_LED_1", analogDiscovery, 0, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Front_LED_2", analogDiscovery, 1, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Front_LED_3", analogDiscovery, 2, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Front_LED_4", analogDiscovery, 3, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Front_LED_5", analogDiscovery, 4, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Relais_K109", analogDiscovery, 10, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Relais_K108", analogDiscovery, 11, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Relais_K104", analogDiscovery, 12, GPIO::DirectionOut, false));

	// Load is switched thru a multiplexer. Speaker outputs can not shortcirquid!
	// So we have one nEn and two ADR lines here
	gpios.push_back(createGPIO("ADR0", analogDiscovery, 13, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("ADR1", analogDiscovery, 14, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("nEnable", analogDiscovery, 15, GPIO::DirectionOut, true));

	// MinnowBoard GPIOs
	// Outputs
	gpios.push_back(createGPIO("Volume Button +", 477, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Volume Button -", 478, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Enc3", 479, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Station1 Button", 472, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Station2 Button", 473, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Station3 Button", 485, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Station4 Button", 475, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Power Button", 484, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Reset Button", 474, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Setup Button", 338, GPIO::DirectionOut, false));
	// Inputs
	gpios.push_back(createGPIO("Led 1", 509, GPIO::DirectionIn, false));
	gpios.push_back(createGPIO("Led 2", 340, GPIO::DirectionIn, false));
	gpios.push_back(createGPIO("Led 3", 505, GPIO::DirectionIn, false));
	gpios.push_back(createGPIO("Led 4", 339, GPIO::DirectionIn, false));
	gpios.push_back(createGPIO("Led 5", 504, GPIO::DirectionIn, false));

	return gpios;
}



int main(int argc, char *argv[])
{

#if 0
	// Plot points
	std::ostream_iterator<double> iter(std::cout, "\n");
	std::copy(points.begin(), points.end(), iter);

	// Save points in points.txt
	std::ofstream outfile("points.txt", std::ofstream::out);
	if (!outfile.is_open()) {
		std::cout << "File not opened!" << std::endl;
	}
	Indexer<double>::setStartIndex(0);
	std::ostream_iterator<Indexer<double>> it(outfile, "\n");
	std::copy(points.begin(), points.end(), it);
	outfile.flush();
	outfile.close();
#endif


	auto devs = AnalogDiscovery::getDevices();
	if (devs.empty()) {
		std::cout << "No devices Found!" << std::endl;
		exit(0);
	}

	std::cout << "Found " << devs.size() << " devices. Opening " <<
				 devs.front().id << " [" << devs.front().ver << "]" << std::endl;

	std::shared_ptr<AnalogDiscovery> sharedDev(new AnalogDiscovery(devs.front()));

	auto gpios = loadGPIOMapping(sharedDev);
	manualTest(gpios);



#if 0
	std::cout << "GPIO1 is: " << (gpio1->getDirection() == GPIO::DirectionIn ? "Input" : "Output") << std::endl;
	std::cout << "GPIO1 is: " << (gpio1->getValue() ? "Hi" : "Lo") << std::endl;
	std::cout << "GPIO2 is: " << (gpio2->getDirection() == GPIO::DirectionIn ? "Input" : "Output") << std::endl;
	std::cout << "GPIO2 is: " << (gpio2->getValue() ? "Hi" : "Lo") << std::endl;

	std::cout << "Press Key" << std::endl;
	getchar();

	for (int i=0; i<4; i++) {

		std::cout << "gpio2->setValue(true);" << std::endl;
		gpio2->setValue(true);
		std::cout << "gpio1->setValue(true);" << std::endl;
		gpio1->setValue(true);

		getchar();

		std::cout << "gpio2->setValue(false);" << std::endl;
		gpio2->setValue(false);
		std::cout << "gpio1->setValue(false);" << std::endl;
		gpio1->setValue(false);

		getchar();

	}

	std::cout << "Behind loop" << std::endl;
#endif

	getchar();

	return 0;



#if 0

	return 0;

	auto terminate = createTerminateFlag();
	std::thread t1(run, terminate);

	std::cout << "Press q to exit..." << std::endl;
	while(getchar() != 'q') {



	}
	terminate->store(true);
	t1.join();

	return 0;
#endif

}
