#include "speaker.h"

Speaker::Speaker()
{
}


// Static
void Speaker::setChannel(SharedGPIOHandle enable, SharedGPIOHandle adr0, SharedGPIOHandle adr1, Channel sp) {
	enable->setValue(false);
	if (sp == Speaker::Hi) {
		adr0->setValue(true);
		adr1->setValue(false);
	} else if (sp == Speaker::Lo) {
		adr0->setValue(true);
		adr1->setValue(true);
	} else if (sp == Speaker::Mid) {
		adr0->setValue(false);
		adr1->setValue(true);
	}
	enable->setValue(true);
}
