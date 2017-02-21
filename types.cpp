#include "types.h"

#include <atomic>

SharedTerminateFlag createSharedTerminateFlag()
{
	return SharedTerminateFlag(new std::atomic<bool>(false));
}

SharedCalibrateAmout createSharedCalibrateAmount()
{
	return SharedCalibrateAmout(new std::atomic<double>(0.0));
}

SharedCommandFlag createSharedCommandFlag()
{
	return SharedCommandFlag(new std::atomic<char>(0));
}
