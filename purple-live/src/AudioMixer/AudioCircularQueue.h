#pragma once

#include <vector>
#include <queue>
#include <mutex>

class AudioCircularQueue
{
public:
	AudioCircularQueue(int capability);
	virtual ~AudioCircularQueue();

	void SetCapability(int capability);
	size_t GetSize(bool lock=true);
	size_t Queue(std::vector<uint8_t>&& buf, bool lock=true);
	size_t Queue(const uint8_t*, int size);
	size_t Dequeue(uint8_t*, int size, bool lock=true);

private:
	std::vector<std::vector<uint8_t>> m_queue;
	std::mutex m_mutex;
	size_t m_capability = 0;
	size_t m_head = 0;
};
