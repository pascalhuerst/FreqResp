#include "encoder.h"
#include "types.h"
#include "debug.h"

#include <thread>


Encoder::Encoder(SharedGPIOHandle a, SharedGPIOHandle b) :
	m_a(a),
	m_b(b),
	m_buffer(std::shared_ptr<BlockingCircularBuffer<Op>>(new BlockingCircularBuffer<Op>("EncoderBuffer", 32))),
	m_terminateRequest(createSharedTerminateFlag()),
	m_thread(new std::thread(Encoder::doWork, m_terminateRequest, m_a, m_b, m_buffer))
{
}

Encoder::~Encoder()
{
	m_terminateRequest->store(true);
	m_thread->join();

	delete m_thread;
}

void Encoder::increment()
{
	Op b = Increment;
	m_buffer->set(&b, 1);
}

void Encoder::decrement()
{
	Op b = Decrement;
	m_buffer->set(&b, 1);
}

//Static
void Encoder::doWork(SharedTerminateFlag terminateRequest, SharedGPIOHandle a, SharedGPIOHandle b,std::shared_ptr<BlockingCircularBuffer<Op>> buffer)
{
	Op localBuffer;

	while (!terminateRequest->load()) {
		// Blocks if nothing to read
		buffer->get(&localBuffer, 1);

		if (localBuffer == Increment)
			Encoder::doIncrement(a,b);
		else if(localBuffer == Decrement)
			Encoder::doDecrement(a,b);
	}
}

//Static
void Encoder::doDecrement(SharedGPIOHandle a, SharedGPIOHandle b)
{
	Debug::error("Encoder::doDecrement", "");
	a->setValue(!a->getValue());
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	b->setValue(!b->getValue());
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	a->setValue(!a->getValue());
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	b->setValue(!b->getValue());
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

//Static
void Encoder::doIncrement(SharedGPIOHandle a, SharedGPIOHandle b)
{
	Debug::error("Encoder::doIncrement", "");
	b->setValue(!b->getValue());
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	a->setValue(!a->getValue());
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	b->setValue(!b->getValue());
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	a->setValue(!a->getValue());
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
