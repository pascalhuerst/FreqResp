#include "measurement.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <numeric>
#include <map>
#include <utility>

// GPIO foo
std::list<SharedGPIOHandle> loadDefaultGPIOMapping(SharedAnalogDiscoveryHandle analogDiscovery)
{
	std::list<SharedGPIOHandle> gpios;

	// Analog Discovery GPIOs
	// Outputs
	gpios.push_back(createGPIO("Front_LED_1", analogDiscovery, 0, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Front_LED_2", analogDiscovery, 1, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Front_LED_3", analogDiscovery, 2, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Front_LED_4", analogDiscovery, 3, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Front_LED_5", analogDiscovery, 4, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Relais_K109", analogDiscovery, 10, GPIO::DirectionOut, false)); // This is to check, if load resistors are damaged
	gpios.push_back(createGPIO("Relais_K108", analogDiscovery, 11, GPIO::DirectionOut, false)); // 4 or 6 Ohm Load for J103-J105
	gpios.push_back(createGPIO("Relais_K104", analogDiscovery, 12, GPIO::DirectionOut, false));	// 4 or 6 Ohm Load for J100-J102

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

// This is usefull on PC, where we dont have those GPIOs available
std::list<SharedGPIOHandle> loadDummyGPIOMapping(SharedAnalogDiscoveryHandle analogDiscovery)
{
	std::list<SharedGPIOHandle> ret;
	return ret;
}

SharedGPIOHandle getGPIOForName(std::list<SharedGPIOHandle> gpios, const std::string &name)
{
	for (auto it=gpios.begin(); it!=gpios.end(); ++it) {
		if ((*it)->getName() == name)
			return *it;
	}
	std::cout << "GPIO with name \"" << name <<  "\" does not exist!" << std::endl;

	return nullptr;
}

GPIOSnapshot createGPIOSnapshot(std::list<SharedGPIOHandle> gpios)
{
	GPIOSnapshot ret;
	for (auto it = gpios.begin(); it != gpios.end(); ++it) {
		GPIOState state;
		state.direction = (*it)->getDirection();
		state.value = (*it)->getValue();
		ret.insert(std::make_pair(*it, state));
	}

	return ret;
}

void updateGPIOSnapshot(GPIOSnapshot *snapshot, SharedGPIOHandle gpio, GPIOState state)
{
	if (!gpio) {
		std::cout << "Ignoring snapshot update. GPIO is nullptr" << std::endl;
		return;
	}

	auto it = snapshot->find(gpio);
	if (it == snapshot->end()) {
		if (snapshot->insert(std::make_pair(gpio, state)).second == false)
			std::cout << "Error inserting: " << gpio->getName() << std::endl;
	} else {
		//std::cout << "Updating " << gpio->getName() << " to " << state.value << std::endl;
		it->second = state;
	}
}

void setGPIOSnapshot(GPIOSnapshot snapshot)
{
	//std::cout << "setSnapshot: have " << snapshot.size() << " values:" << std::endl;
	for (auto it=snapshot.begin(); it!=snapshot.end(); it++) {
		//std::cout << "  updating: " << it->first->getName() << " to: " << it->second.value << std::endl;
		it->first->setDirection(it->second.direction);
		it->first->setValue(it->second.value);
	}
}

// Class
Measurement::Measurement(const std::string &name, SharedAnalogDiscoveryHandle dev, GPIOSnapshot gpioSnapshot, double fMin, double fMax, int pointsPerDecade) :
	m_name(name),
	m_dev(dev),
	m_gpioSnapshot(gpioSnapshot),
	m_isRunning(false),
	m_terminateRequest(createSharedTerminateFlag()),
	m_thread(nullptr),
	m_fMin(fMin),
	m_fMax(fMax),
	m_pointsPerDecade(pointsPerDecade)
{
}

Measurement::~Measurement()
{
	stop();
}

void Measurement::start(int channelId)
{
	if (m_isRunning) {
		std::cout << "Measurement " << m_name << " is already running. Ignoring start command" << std::endl;
		return;
	}

	setGPIOSnapshot(m_gpioSnapshot);

	m_terminateRequest->store(false);
	m_thread = new std::thread(Measurement::run, m_terminateRequest, m_dev, channelId, this);
	m_isRunning = true;
}

void Measurement::stop()
{
	if (!m_isRunning) {
		std::cout << "Measurement " << m_name << " is not running. Ignoring stop command" << std::endl;
		return;
	}

	m_terminateRequest->store(true);
	m_thread->join();

	delete m_thread;
	m_isRunning = false;
}

bool Measurement::isRunning()
{
	if (m_terminateRequest->load())
		stop();

	return m_isRunning;
}

double Measurement::dBuForVolts(double v) {
	static const double Uref = 0.7746;
	return 20 * log(v / Uref);
}

double Measurement::dBvForVolts(double v) {
	static const double Uref = 1.0;
	return 20 * log (v / Uref);
}

double Measurement::rms(const std::vector<double>& samples)
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
std::vector<double> Measurement::createMeasuringPoints(int pointsPerDecade, double minHz, double maxHz)
{
	// If requested points are within one decade, extend range first
	// aditional values will be removed later on
	double maxHzFixed = (maxHz / minHz < 10.f ? maxHz * 10.f : maxHz);

	// e Values per decade
	double e = static_cast<double>(pointsPerDecade);
	double k = std::pow(10, 1.0 / e);

	std::vector<double> points;
	for (int exp=0; exp<e; exp++) {
		//double currentVal = round(pow(k, exp) * 10.0) / 10.0;
		double currentVal = (pow(k, exp) * 10.0) / 10.0;
		points.push_back(currentVal);

		double tmpMax = maxHzFixed*10.0;
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

	return points;
}

SharedTerminateFlag Measurement::createSharedTerminateFlag()
{
	return SharedTerminateFlag(new std::atomic<bool>(false));
}

void Measurement::saveBuffer(const std::vector<double>& s, const std::string& fileName)
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


// Static
void Measurement::run(SharedTerminateFlag terminateRequest, SharedAnalogDiscoveryHandle dev, int channelId, Measurement *ptr)
{
	// Create frequency Map with measuring frequencies
	auto points = ptr->createMeasuringPoints(ptr->m_pointsPerDecade, ptr->m_fMin, ptr->m_fMax);

	ptr->saveBuffer(points, "fMeasuringPoints.txt");

	exit(0);

	std::vector<double>freqResp(points.size());

	try {
		dev->setAnalogOutputAmplitude(channelId, 5); // 200mV

		//dev->setAnalogOutputAmplitude(channelId, 0.2); // 200mV
		dev->setAnalogOutputWaveform(channelId, AnalogDiscovery::WaveformSine);

		auto currentFrequency = points.begin();
		auto currentFreqResp = freqResp.begin();

		while (!terminateRequest->load() && currentFrequency != points.end()) {

			dev->setAnalogOutputFrequency(channelId, *currentFrequency);
			dev->setAnalogOutputEnabled(channelId, true);
			std::this_thread::sleep_for(std::chrono::milliseconds(50));

			auto samples = readOneBuffer(dev, channelId, *currentFrequency);

			// Remove upper and lower 10% leads to better results
			int removeCount = samples.size() * 0.1;
			samples.erase(samples.begin(), samples.begin()+removeCount);
			samples.erase(samples.end()-removeCount, samples.end());

			std::vector<double> keys(samples.size());
			std::iota(std::begin(keys), std::end(keys), 0);

			*currentFreqResp = ptr->dBuForVolts(ptr->rms(samples));

			std::cout << "[" << std::distance(points.begin(), currentFrequency)
					  << " ch=" << channelId
					  << "] dBu @ " << double(*currentFrequency)
					  << "Hz: " << *currentFreqResp << std::endl;

			currentFrequency++;
			currentFreqResp++;

		}
	} catch(AnalogDiscoveryException e) {
		std::cerr << e.what() << std::endl;
	} catch (std::exception e) {
		std::cerr << e.what() << std::endl;
	}

	terminateRequest->store(true);

	ptr->saveBuffer(freqResp, "measurement.txt");
}
