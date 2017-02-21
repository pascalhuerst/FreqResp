#pragma once

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>
#include <cmath>
#include <cstring>

template <typename T>
class BlockingCircularBuffer
{
public:
	BlockingCircularBuffer(const std::string& name, int size) :
		m_buffer(nullptr),
		m_size(size),
		m_name(name),
		m_bytesRead(0),
		m_bytesWritten(0),
		m_readIndex(0),
		m_writeIndex(0)
	{
		init();
	}

	~BlockingCircularBuffer()
	{
		if (m_buffer)
			delete [] m_buffer;
	}

	void get(T *buffer, unsigned int size)
	{
		if (!m_buffer) {
			std::cerr << "Buffer (" << m_name << ") not initialized!" << std::endl;
		}

		std::cout << "availableToRead=" << availableToRead() << " availableToWrite" << availableToWrite() << std::endl;

		std::unique_lock<std::mutex> mlock(m_mutex);
		while (availableToRead() < size || !m_buffer) {
			m_condition.wait(mlock);
		}

		m_bytesRead += size;

		for (unsigned int i=0; i<size; i++) {
			m_readIndex++;
			m_readIndex = m_readIndex % m_size;
			buffer[i] = m_buffer[m_readIndex];
		}

		m_condition.notify_one();
	}

	void set(T *buffer, unsigned int size)
	{
		if (!m_buffer) {
			std::cerr << "Buffer (" << m_name << ") not initialized!" << std::endl;
		}

		std::unique_lock<std::mutex> mlock(m_mutex);
		while (availableToWrite() < size || !m_buffer) {
			m_condition.wait(mlock);
		}


		m_bytesWritten += size;

		for (unsigned int i=0; i<size; i++) {
			//std::cout << "Writing " << buffer[i] << " to blocking buffer" << std::endl;
			m_writeIndex++;
			m_writeIndex = m_writeIndex % m_size;
			m_buffer[m_writeIndex] = buffer[i];
		}

		m_condition.notify_one();
	}

	inline unsigned int availableToRead() const
	{
		return m_size - availableToWrite();
	}

	inline unsigned int availableToWrite() const
	{
		int tmp = m_readIndex - m_writeIndex - 1;
		int distance = tmp < 0 ? -tmp : tmp;

		return m_writeIndex < m_readIndex ? distance : m_size - distance;
	}

	inline int size() const
	{
		return m_size;
	}

	inline std::string name() const
	{
		return m_name;
	}

private:
	T *m_buffer;
	std::atomic<int> m_size;
	std::string m_name;

	std::mutex m_mutex;
	std::condition_variable m_condition;

	std::atomic<unsigned long> m_bytesRead;
	std::atomic<unsigned long> m_bytesWritten;

	std::atomic<unsigned int> m_readIndex;
	std::atomic<unsigned int> m_writeIndex;

	void init()
	{
		std::unique_lock<std::mutex> mlock(m_mutex);

		if (m_buffer)
			delete [] m_buffer;

		m_buffer = new T[m_size];
		memset(m_buffer, 0, m_size);

		m_bytesRead = 0;
		m_bytesWritten = 0;
		m_readIndex = 0;
		m_writeIndex = 0;

		m_condition.notify_one();
	}

};

