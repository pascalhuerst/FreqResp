#if 0

#include "Waveform.h"
#include "Device.h"
#include <math.h>
#include <new>

Waveform::Waveform(void)
{
	this->num_samples = 0;
}

// Generate data by sweeping from start frequency to end frequency for X number
// of samples
Waveform::Waveform(double start_freq, double end_freq, int num_samples)
{
	double startl, endl, interval, delta;
	int i;
	Device dev;

	if (!dev.isOpen()) {
		this->num_samples = 0;
		return;
	}
	dev.startAcquisition(5);
	dev.enableOutput(1.41);

	this->num_samples = num_samples;

	// TODO: Currently only logarithmic scale supported,
	// probably need to add support for linear scale as well.
	// Hint: Add the scale type enum as an argument to constructor
	startl = log10(start_freq);
	endl= log10(end_freq);

	delta = (endl - startl) / (num_samples - 1);
	for (i = 0; i < (int) num_samples; i++) {
		double freq = pow(10, startl);
		double volt_rms = 0xFFFF; // Invalid

		// Take 32 samples of each period
		std::cout << "1..." << std::endl;
		if (!dev.read_rms(freq, (int)freq * 32, volt_rms)) {
			std::cout << "Error reading RMS value for freq: " << freq << std::endl << std::flush;
			break;
		}
		std::cout << "2..." << std::endl;

		this->frequencies.push_back(freq);
		this->volt_rms.push_back(volt_rms);

		startl += delta;
	}
}

Waveform::Waveform(const char *waveform_data_file)
{
	std::string line;
	ifstream file(waveform_data_file);

	double frequency, volt_rms;
	std::string comma;

	this->num_samples = 0;
	while (file >> frequency >> comma >> volt_rms) {
		this->num_samples++;
		this->frequencies.push_back(frequency);
		this->volt_rms.push_back(volt_rms);
	}

	file.close();
}

Waveform::Waveform(int num_samples)
{
	this->num_samples = num_samples;
	this->frequencies.reserve(num_samples);
	this->volt_rms.reserve(num_samples);
}

Waveform::~Waveform(void)
{
	/* Place holder */
}

void Waveform::set_ref_volt(double volt)
{
	this->volt_ref = volt;
}

int Waveform::get_num_samples(void)
{
	return this->num_samples;
}

int Waveform::write_to_file(const char *filename)
{
	ofstream target_file;

	if (this->num_samples == 0) {
		std::cout << "No Samples available for this waveform, skip writing to file." << std::endl << std::flush;
		return -1;
	}

	target_file.open(filename);
	if (!target_file.is_open())
		return -1;

	std::cout << "Writing " << this->num_samples << " sample information to file... " << std::flush;
	for (int i = 0; i < this->num_samples; i++)
		target_file << this->frequencies[i] << ", " << this->volt_rms[i] << std::endl;

	target_file.close();

	std::cout << "Done\n" << std::flush;
	return 0;
}

void Waveform::dump(void)
{
	std::cout << "Dumping " << this->num_samples << " samples information\n" << std::flush;
	for (int i = 0; i < this->num_samples; i++)
		std::cout << "Freq: " << this->frequencies[i]
			  << "Hz\tV(rms): " << this->volt_rms[i]
			  << std::endl << std::flush;

}

void Waveform::resize(int num_samples)
{
	if (this->num_samples == num_samples)
		return;

	this->num_samples = num_samples;
	this->frequencies.resize(num_samples);
	this->volt_rms.resize(num_samples);
}

void Waveform::set_freq_at(double freq, int index)
{
	if (index < this->num_samples)
		this->frequencies[index] = freq;
}

void Waveform::set_vrms_at(double vrms, int index)
{
	if (index < this->num_samples)
		this->volt_rms[index] = vrms;
}

bool Waveform::get_vrms_at(double& vrms, int index)
{
	if (index >= this->num_samples)
		return false;

	vrms = this->volt_rms[index];
	return true;
}

bool Waveform::get_freq_at(double& freq, int index)
{
	if (index >= this->num_samples)
		return false;

	freq = this->frequencies[index];
	return true;
}

bool Waveform::get_deviation(Waveform w1, Waveform w2, Waveform& w)
{
	if (w1.get_num_samples() != w2.get_num_samples()) {
		std::cout << "Mismatch in number of samples of two waveforms\n" << std::flush;
		return false;
	}

	int num_samples = w1.get_num_samples();
	w.resize(num_samples);

	for (int i = 0; i < num_samples; i++) {
		double w1_freq, w2_freq;
		w1.get_freq_at(w1_freq, i);
		w2.get_freq_at(w2_freq, i);
		w.set_freq_at(w1_freq - w2_freq, i);

		double w1_vrms, w2_vrms;
		w1.get_vrms_at(w1_vrms, i);
		w2.get_vrms_at(w2_vrms, i);
		w.set_vrms_at(w1_vrms - w2_vrms, i);
		//printf("Difference of vrms at freq :%lf is (%lf - %lf) = %lf\n",
		//		w1_freq, w1_vrms, w2_vrms, w1_vrms - w2_vrms);
	}
	return true;
}

#endif
