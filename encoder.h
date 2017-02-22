#pragma once

#include "types.h"
#include "gpio.h"
#include "blockingcircularbuffer.h"

namespace std {
	class thread;
}

class Encoder
{
public:
	Encoder(SharedGPIOHandle a, SharedGPIOHandle b);
	~Encoder();

	void increment();
	void decrement();

	enum Op {
		Increment,
		Decrement
	};

private:
	static void doWork(SharedTerminateFlag terminateRequest, SharedGPIOHandle a, SharedGPIOHandle b, std::shared_ptr<BlockingCircularBuffer<Op> > buffer);
	static void doIncrement(SharedGPIOHandle a, SharedGPIOHandle b);
	static void doDecrement(SharedGPIOHandle a, SharedGPIOHandle b);

	SharedGPIOHandle m_a;
	SharedGPIOHandle m_b;
	std::shared_ptr<BlockingCircularBuffer<Op>> m_buffer;
	SharedTerminateFlag m_terminateRequest;

	std::thread *m_thread;
};

