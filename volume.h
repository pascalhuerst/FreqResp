#pragma once

#include "types.h"
#include "encoder.h"

// Some devices have buttons to set volume, some have an encoder.
// This class abstracts those differences.

class Volume
{
public:
	Volume();
	virtual ~Volume();

	virtual void up() = 0;
	virtual void down() = 0;
};

typedef std::shared_ptr<Volume> SharedVolumeHandle;

class VolumeButtons : public Volume
{
public:
	typedef Volume basetype;

	VolumeButtons(SharedGPIOHandle up, SharedGPIOHandle down);
	virtual ~VolumeButtons();

	virtual void up();
	virtual void down();
private:
	SharedGPIOHandle m_gpioUp;
	SharedGPIOHandle m_gpioDown;
};

class VolumeEncoder : public Volume
{
public:
	typedef Volume basetype;

	VolumeEncoder(SharedGPIOHandle a, SharedGPIOHandle b);
	virtual ~VolumeEncoder();

	virtual void up();
	virtual void down();
private:
	Encoder m_encoder;
};

SharedVolumeHandle createVolume(SharedGPIOHandle a, SharedGPIOHandle b, bool isEncoder);

