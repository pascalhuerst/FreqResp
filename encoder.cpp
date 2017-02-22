#include "encoder.h"
#include "types.h"
#include "debug.h"

#include <thread>
#include <chrono>

using namespace std::chrono_literals;

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
	if (!m_buffer->set(&b, 1, 1s))
		Debug::warning("Encoder::increment", "Can not enqueue increment command!");
}

void Encoder::decrement()
{
	Op b = Decrement;
	if (!m_buffer->set(&b, 1, 1s))
		Debug::warning("Encoder::increment", "Can not enqueue decrement command!");
}

//Static
void Encoder::doWork(SharedTerminateFlag terminateRequest, SharedGPIOHandle a, SharedGPIOHandle b, std::shared_ptr<BlockingCircularBuffer<Op> > buffer)
{
	Op localBuffer;

	while (!terminateRequest->load()) {
		// Blocks for 1 second if nothing to read
		// returns false if timeout occured
		if (!buffer->get(&localBuffer, 1, 1s))
			continue;

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
	std::this_thread::sleep_for(20ms);
	b->setValue(!b->getValue());
	std::this_thread::sleep_for(20ms);
	a->setValue(!a->getValue());
	std::this_thread::sleep_for(20ms);
	b->setValue(!b->getValue());
	std::this_thread::sleep_for(20ms);
}

//Static
void Encoder::doIncrement(SharedGPIOHandle a, SharedGPIOHandle b)
{
	Debug::error("Encoder::doIncrement", "");
	b->setValue(!b->getValue());
	std::this_thread::sleep_for(20ms);
	a->setValue(!a->getValue());
	std::this_thread::sleep_for(20ms);
	b->setValue(!b->getValue());
	std::this_thread::sleep_for(20ms);
	a->setValue(!a->getValue());
	std::this_thread::sleep_for(20ms);
}
