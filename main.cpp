#include <iostream>
#include <thread>
#include <fstream>
#include <iterator>
#include <cmath>
#include <algorithm>

#include "Device.h"

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
	return rms;
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

// create logarithmically well distributed measuring points,
// so we have the same amount of measuring points in each decade.
void createMeasuringPoints(std::vector<double>& points, int pointsPerDecade, double minHz, double maxHz)
{
	// e Values per decade
	double e = static_cast<double>(pointsPerDecade);
	double k = std::pow(10, 1.0 / e);

	for (int exp=0; exp<e; exp++) {
		double currentVal = round(pow(k, exp) * 10.0) / 10.0;
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

void run(SharedTerminateFlag terminateRequest)
{
	auto devs = Device::getDevices();
	if (devs.empty()) {
		std::cout << "No devices Found!" << std::endl;
		exit(0);
	}

	std::cout << "Found " << devs.size() << " devices. Opening " <<
				 devs.front().id << " [" << devs.front().ver << "]" << std::endl;

	Device dev(devs.front());

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
		dev.setAnalogOutputWaveform(channel, Device::WaveformSine);

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
			w->updateSineGraph(keys, samples);

			*currentFreqResp = dBuForVolts(rms(samples));
			w->updateFreqGraph(points, freqResp);

			std::cout << "[" << std::distance(points.begin(), currentFrequency)
					  << "] dBu @ " << double(*currentFrequency)
					  << "Hz: " << *currentFreqResp << std::endl;

			currentFrequency++;
			currentFreqResp++;

		}
	} catch(DeviceException e) {
		std::cout << "e.what(): " << e.what() << std::endl;
	} catch (std::exception e) {
		std::cout << "e.what(): " << e.what() << std::endl;
	}
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


	// Create frequency Map with measuring frequencies
	std::vector<double>points;
	double fMin = 20;
	double fMax = 20000;
	int pointsPerDecade = 100;
	createMeasuringPoints(points, pointsPerDecade, fMin, fMax);

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


	try {
		Device dev(devs.front());
		std::cout << "Version: " << dev.version() << std::endl;

		auto terminate = createTerminateFlag();
		SharedSampleStorage samples(new std::vector<std::vector<double>>(points.size()));

		std::thread t1(readOneBuffer, &dev, points, samples, terminate);

		while (!terminate->load()) {

			//std::cout << "InputStatus:  " << dev.analogInputStatus() << std::endl;
			//std::cout << "OutputStatus: " << dev.analogOutputStatus() << std::endl;
			for (auto i = samples->begin(); i!= samples->end(); ++i) {
				if (!i->empty())
					std::cout << "Samples [" << std::distance(samples->begin(),i) << "] :" << i->size() << std::endl;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}

		terminate->store(true);
		t1.join();

		auto s = samples->begin();
		auto p = points.begin();
		char buf[3];
		std::vector<PrintablePair<double,double>> freqResp;

		for (unsigned i = 0; s != samples->end(); s++, p++, i++) {
			sprintf(buf, "%03i", i);
			saveBuffer(*s, "sampledFreq_" + std::string(buf) + "_" + std::to_string(*p));

			// Creates a vector containing frequency response values as ( first=freq | second=RMS )
			freqResp.push_back(
						// Find max and divide by sqrt(2) to get RMS
						//PrintablePair<double,double>(*p,dBuForVolts(*max_element(s->begin(), s->end()) / sqrt(2))));
						// calculate RMS from samples
						PrintablePair<double,double>(*p, dBuForVolts(rms(*s))));

			std::ofstream outfile("resp.txt", std::ofstream::out);

			if (!outfile.is_open()) {
				std::cout << "File not opened!" << std::endl;
			}
			std::ostream_iterator<PrintablePair<double,double>> iter(outfile, "\n");
			std::copy(freqResp.begin(), freqResp.end(), iter);
		}

	} catch (DeviceException e) {
		std::cout << "e.what(): " << e.what() << std::endl;
	} catch (std::exception e) {
		std::cout << "e.what(): " << e.what() << std::endl;
	}

	return 0;
}
