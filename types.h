#pragma once

#include <memory>
#include <vector>

typedef std::shared_ptr<std::vector<std::vector<double>>> SharedSampleStorage;
typedef std::shared_ptr<std::atomic<bool>> SharedTerminateFlag;
typedef std::shared_ptr<std::atomic<double>> SharedCalibrateAmout;
typedef std::shared_ptr<std::atomic<char>> SharedCommandFlag;

SharedTerminateFlag createSharedTerminateFlag();
SharedCalibrateAmout createSharedCalibrateAmount();
SharedCommandFlag createSharedCommandFlag();
