#if 0

#include "Waveform.h"
#include "Device.h"

int main(int argc, char *argv[])
{

	// Sweep from 20hz to 20khz
	Waveform wave1(20,2000,50);
//	Waveform wave2(argv[2]);

	wave1.write_to_file(argv[1]);
//	wave2.write_to_file(argv[2]);

//	Waveform w(50);
//	if (w.get_deviation(wave1, wave2, w))
//		w.write_to_file(argv[2]);
//	else
//		std::cout << "Cannot find deviation\n" << std::flush;

	// Read data from file
	//	Waveform wave(argv[1]);
	//	wave.dump();
#if 0
	// Read the RMS value for a particular freq
	Device d;
	d.enable_input(5);
	d.enable_output(1.41);

	double freq = 15806;
	int samples = freq * 32;
	double rms_out;

	if (!d.read_rms(freq, samples, rms_out))
		std::cout << "Error reading RMS value for freq: " << freq << std::endl << std::flush;
	else
		std::cout << "Freq: " << freq << "\tRMS: " << rms_out << std::endl << std::flush;
	return 0;
#endif
}


#endif
