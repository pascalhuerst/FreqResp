#include "measurement.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <numeric>
#include <map>
#include <utility>

#include "debug.h"

// GPIO foo
std::list<SharedGPIOHandle> loadDefaultGPIOMapping(SharedAnalogDiscoveryHandle analogDiscovery)
{
	std::list<SharedGPIOHandle> gpios;

	// Some Mappings are Speaker dependent :(

#if 0
	// #### MAPPING AS IN SCHEMATICS (DEFAULT) ####
	// Analog Discovery GPIOs
	// Outputs
	gpios.push_back(createGPIO("Front_LED_1", analogDiscovery, 0, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Front_LED_2", analogDiscovery, 1, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Front_LED_3", analogDiscovery, 2, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Front_LED_4", analogDiscovery, 3, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Front_LED_5", analogDiscovery, 4, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Relais_Power", analogDiscovery, 9, GPIO::DirectionOut, true)); // This is the main Power for the speaker
	gpios.push_back(createGPIO("Relais_K109", analogDiscovery, 10, GPIO::DirectionOut, false)); // This is to check, if load resistors are damaged
	gpios.push_back(createGPIO("Relais_K108", analogDiscovery, 11, GPIO::DirectionOut, false)); // 4 or 6 Ohm Load for J103-J105
	gpios.push_back(createGPIO("Relais_K104", analogDiscovery, 12, GPIO::DirectionOut, false));	// 4 or 6 Ohm Load for J100-J102

	// Load is switched thru a multiplexer. Speaker outputs can not shortcirquid!
	// So we have one nEn and two ADR lines here
	gpios.push_back(createGPIO("ADR0", analogDiscovery, 13, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("ADR1", analogDiscovery, 14, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Enable", analogDiscovery, 15, GPIO::DirectionOut, false));

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
	gpios.push_back(createGPIO("Reset Button", 474, GPIO::DirectionOut, true)); //Investigate: This must be true, othewise speaker tries to flash. Inverted?
	gpios.push_back(createGPIO("Setup Button", 338, GPIO::DirectionOut, false));
	// Inputs
	gpios.push_back(createGPIO("Led 1", 509, GPIO::DirectionIn, false));
	gpios.push_back(createGPIO("Led 2", 340, GPIO::DirectionIn, false));
	gpios.push_back(createGPIO("Led 3", 505, GPIO::DirectionIn, false));
	gpios.push_back(createGPIO("Led 4", 339, GPIO::DirectionIn, false));
	gpios.push_back(createGPIO("Led 5", 504, GPIO::DirectionIn, false));
#endif
#if 1
	// #### Stereo Cubes Mapping ####
	// Analog Discovery GPIOs
	// Outputs
	gpios.push_back(createGPIO("Front_LED_1", analogDiscovery, 0, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Front_LED_2", analogDiscovery, 1, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Front_LED_3", analogDiscovery, 2, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Front_LED_4", analogDiscovery, 3, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Front_LED_5", analogDiscovery, 4, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Relais_Power", analogDiscovery, 9, GPIO::DirectionOut, true)); // This is the main Power for the speaker
	gpios.push_back(createGPIO("Relais_K109", analogDiscovery, 10, GPIO::DirectionOut, false)); // This is to check, if load resistors are damaged
	gpios.push_back(createGPIO("Relais_K108", analogDiscovery, 11, GPIO::DirectionOut, false)); // 4 or 6 Ohm Load for J103-J105
	gpios.push_back(createGPIO("Relais_K104", analogDiscovery, 12, GPIO::DirectionOut, false));	// 4 or 6 Ohm Load for J100-J102

	// Load is switched thru a multiplexer. Speaker outputs can not shortcirquid!
	// So we have one nEn and two ADR lines here
	gpios.push_back(createGPIO("ADR0", analogDiscovery, 13, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("ADR1", analogDiscovery, 14, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Enable", analogDiscovery, 15, GPIO::DirectionOut, false));


#warning "!!!! Reenable MinnowBoard GPIOs !!!!"
#if 0
	// MinnowBoard GPIOs
	// Outputs
	gpios.push_back(createGPIO("Enc3", 479, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Volume Button +", 472, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Volume Button -", 485, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Power Button", 484, GPIO::DirectionOut, false));
	gpios.push_back(createGPIO("Reset Button", 474, GPIO::DirectionOut, true)); //Investigate: This must be true, othewise speaker tries to flash. Inverted?
	gpios.push_back(createGPIO("Setup Button", 338, GPIO::DirectionOut, false));
	// Inputs
	gpios.push_back(createGPIO("Led 1", 509, GPIO::DirectionIn, false));
	gpios.push_back(createGPIO("Led 2", 340, GPIO::DirectionIn, false));
	gpios.push_back(createGPIO("Led 3", 505, GPIO::DirectionIn, false));
	gpios.push_back(createGPIO("Led 4", 339, GPIO::DirectionIn, false));
	gpios.push_back(createGPIO("Led 5", 504, GPIO::DirectionIn, false));
#endif
#endif

	return gpios;
}

// This is usefull on PC, where we dont have those GPIOs available
std::list<SharedGPIOHandle> loadDummyGPIOMapping(SharedAnalogDiscoveryHandle analogDiscovery)
{
	std::list<SharedGPIOHandle> ret;
	return ret;
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
		Debug::warning("updateGPIOSnapshot", "Ignoring snapshot update. GPIO(" + gpio->getName() + ") is nullptr");
		return;
	}

	auto it = snapshot->find(gpio);
	if (it == snapshot->end()) {
		if (snapshot->insert(std::make_pair(gpio, state)).second == false)
			Debug::warning("updateGPIOSnapshot", "Can not insert: " + gpio->getName());
	} else {
		Debug::verbose("updateGPIOSnapshot", "Updating " + gpio->getName() + " to " + std::to_string(state.value));
		it->second = state;
	}
}

void setGPIOSnapshot(GPIOSnapshot snapshot)
{
	Debug::verbose("Measurement", "setGPIOSnapshot: Setting " + std::to_string(snapshot.size()) + " values!");
	for (auto it=snapshot.begin(); it!=snapshot.end(); it++) {
		Debug::verbose("setGPIOSnapshot", "Setting: " + it->first->getName() + " to: " + std::to_string(it->second.value));
		it->first->setDirection(it->second.direction);
		it->first->setValue(it->second.value);
	}
}

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
	}
	rms = sqrt(rms / samples.size());
	return rms;
}

double rms(double vsine)
{
	return vsine / sqrt(2);
}

void saveBuffer(const std::vector<double>& s, const std::string& fileName)
{
	std::ofstream outfile(fileName, std::ofstream::out);

	if (!outfile.is_open()) {
		Debug::error("saveBuffer", "Can not save buffer! File not opened: " + fileName);
		return;
	}

	IntIndexer<double>::setStartIndex(0);
	std::ostream_iterator<IntIndexer<double>> iter(outfile, "\n");
	std::copy(s.begin(), s.end(), iter);
	outfile.flush();
	outfile.close();
}

void saveMeasurement(const std::vector<double>& frequencies, const std::vector<double>& responses, const std::string& fileName)
{
	Debug::debug("saveMeasurement",  "Saving measurement to file: " + fileName);

	std::ofstream outfile(fileName, std::ofstream::out);

	if (!outfile.is_open()) {
		Debug::error("saveBuffer", "Can not save buffer! File not opened: " + fileName);
		return;
	}

	auto currentResponse = responses.begin();
	auto currentFrequency = frequencies.begin();

	while (currentResponse != responses.end() && currentFrequency != frequencies.end()) {
		outfile << std::setprecision(6) << *currentFrequency << "," << std::setprecision(6) << *currentResponse << std::endl;
		currentResponse++;
		currentFrequency++;
	}

	outfile.flush();
	outfile.close();
}


// Class
Measurement::Measurement(const std::string &name, SharedAnalogDiscoveryHandle dev, double fMin, double fMax, int pointsPerDecade) :
	m_name(name),
	m_dev(dev),
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

void Measurement::start(int channel, double outputCalibration)
{
	Debug::verbose("Measurement::start", "Starting measurement");

	if (m_isRunning) {
		Debug::warning("Measurement", "Already started. Ignoring start command!");
		return;
	}

	m_terminateRequest->store(false);
	m_thread = new std::thread(Measurement::run, m_terminateRequest, m_dev, channel, outputCalibration, this);
	m_isRunning = true;
}

void Measurement::stop()
{
	Debug::verbose("Measurement::stop", "Stopping measurement");

	if (!m_isRunning) {
		Debug::warning("Measurement", "Already stopped. Ignoring stop command!");
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

std::string Measurement::name() const
{
    return m_name;
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

// Static
void Measurement::run(SharedTerminateFlag terminateRequest, SharedAnalogDiscoveryHandle dev, int channel, double outputCalibration, Measurement *ptr)
{
	// Create frequency vector containing measuring frequencies
	auto points = ptr->createMeasuringPoints(ptr->m_pointsPerDecade, ptr->m_fMin, ptr->m_fMax);

	//saveBuffer(points, "fMeasuringPoints.txt");

	std::vector<double>freqResp(points.size());

	try {

		dev->setAnalogOutputAmplitude(channel, 1.08 + outputCalibration); // 1.08Vpp -> 0.77 Vrms -> 0dBu input signal + calibration
		dev->setAnalogOutputWaveform(channel, AnalogDiscovery::WaveformSine);

		auto currentFrequency = points.begin();
		auto currentFreqResp = freqResp.begin();

		while (!terminateRequest->load() && currentFrequency != points.end()) {

			dev->setAnalogOutputFrequency(channel, *currentFrequency);
			dev->setAnalogOutputEnabled(channel, true);

			std::this_thread::sleep_for(std::chrono::milliseconds(50));

			auto samples = readOneBuffer(dev, channel, *currentFrequency);

			// Remove upper and lower 10% leads to better results
			int removeCount = samples.size() * 0.1;
			samples.erase(samples.begin(), samples.begin()+removeCount);
			samples.erase(samples.end()-removeCount, samples.end());

			std::vector<double> keys(samples.size());
			std::iota(std::begin(keys), std::end(keys), 0);

			*currentFreqResp = dBuForVolts(rms(samples));

			Debug::debug("Measurement::run", std::to_string(std::distance(points.begin(), currentFrequency))
						 + " ch=" + std::to_string(channel) + "  "
						 + std::to_string(double(*currentFrequency)) + "Hz: " + std::to_string(*currentFreqResp));

			currentFrequency++;
			currentFreqResp++;
		}

        saveMeasurement(points, freqResp, ptr->name());

	} catch(AnalogDiscoveryException e) {
		std::cerr << e.what() << std::endl;
	} catch (std::exception e) {
		std::cerr << e.what() << std::endl;
	}

	terminateRequest->store(true);

	saveBuffer(freqResp, "measurement.txt");
}

// Static
void Measurement::calibrate(SharedTerminateFlag terminateRequest, SharedCalibrateAmout amount, SharedCommandFlag cmd, SharedAnalogDiscoveryHandle dev)
{
	// It does not matter on which channel (= left/right) we measure to calibrate,
	// since the inputsignal is mono anyways! But we have to switch between woofer, sub and tweeter,
	// to see which has the highest output at 1 kHz
	const int channel = 0;

	try {
		double refFrequency = 1000;	// We want 0dBu @ 1kHz
		double refOutput = 1.08;	// 1.08Vpp -> 0.77 Vrms -> 0dBu input signal

		dev->setAnalogOutputAmplitude(channel, refOutput);
		dev->setAnalogOutputWaveform(channel, AnalogDiscovery::WaveformSine);

		dev->setAnalogOutputFrequency(channel, refFrequency);
		dev->setAnalogOutputEnabled(channel, true);

		while (!terminateRequest->load()) {

			auto samples = readOneBuffer(dev, channel, refFrequency);

			// Remove upper and lower 10% leads to better results
			int removeCount = samples.size() * 0.1;
			samples.erase(samples.begin(), samples.begin()+removeCount);
			samples.erase(samples.end()-removeCount, samples.end());

			std::cout << "output: " << std::to_string(rms(refOutput)) << "Vrms(" << std::to_string(dBuForVolts(rms(refOutput))) << "dBu) --- input: "
					  << std::to_string(rms(samples)) << "Vrms(" <<  std::to_string(dBuForVolts(rms(samples))) << "dBu) ---"
					  << std::to_string(*std::max_element(samples.begin(), samples.end())) << "Vmax" << std::endl;

			// Commands
			char ncmd = cmd->load();
			if (ncmd == 'b') {
				cmd->store(0);
				std::cout << "saving buffer..." << std::endl;
				saveBuffer(samples, "samples.txt");
			}

			// Calibration Amount
			double namount = amount->load();
			if (namount != 0) {
				amount->store(0);
				refOutput += namount;

				dev->setAnalogOutputAmplitude(0, refOutput);
				dev->setAnalogOutputEnabled(0, true);
				dev->setAnalogOutputAmplitude(1, refOutput);
				dev->setAnalogOutputEnabled(1, true);
			}
		}
	} catch(AnalogDiscoveryException e) {
		std::cerr << e.what() << std::endl;
	} catch (std::exception e) {
		std::cerr << e.what() << std::endl;
	}

	terminateRequest->store(true);
}
