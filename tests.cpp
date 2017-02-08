#include "tests.h"
#include "specialkeyboard.h"

#include <iostream>

// This is a bit hackish, but works good to test-toggle GPIOs by hand.
void manualGPIOTest(std::list<SharedGPIOHandle> gpios)
{
	char g = 0;

	// nonblocking keyboard
	SpecialKeyboard kb;

	do {

		int distance = g - 'A';
		if (distance < gpios.size()) {
			auto iter = gpios.begin();
			while(distance--) iter++;
			(*iter)->setValue(!(*iter)->getValue());
		}

		char v = 'A';
		std::cout << "### Use keys on the left to toggle output, 'q' for exit: ###" << std::endl << std::endl;
		for (auto iter = gpios.begin(); iter != gpios.end(); ++iter, v++)
			std::cout << v << "      " << (*iter)->getValue() << "  "
					  << ((*iter)->getDirection() == GPIO::DirectionIn ? "IN   " : "OUT  ")
					  << (*iter)->getName() << std::endl;

		std::cout << std::endl << std::endl;

		while ((g = kb.kbhit()) == 0);
	} while(g != 'q');

}
