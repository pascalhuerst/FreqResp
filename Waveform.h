#if 0

#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

class Waveform {
	private:
		vector<double> frequencies;
		vector<double> volt_rms;
		double volt_ref;
		int num_samples;

	public:
		Waveform(void);
		Waveform(double start_freq, double end_freq, int num_samples);
		Waveform(const char *filename);
		Waveform(int num_samples);
		~Waveform(void);

		void set_ref_volt(double volt);
		int get_num_samples(void);
		int write_to_file(const char *filename);
		void dump();
		void resize(int num_samples);
		bool get_deviation(Waveform w1, Waveform w2, Waveform& w);

		void set_freq_at(double freq, int index);
		bool get_freq_at(double& freq, int index);

		void set_vrms_at(double vrms, int index);
		bool get_vrms_at(double& vrms, int index);
};


#endif
