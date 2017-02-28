#pragma once

#include "gpio.h"

class Speaker
{
public:
	Speaker();

	enum Channel {
		Hi,
		Mid,
		Lo
	};

	static void setChannel(SharedGPIOHandle enable, SharedGPIOHandle adr0, SharedGPIOHandle adr1, Channel sp);

};

