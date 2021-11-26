#include "AudioCircularQueue.h"
#include "easylogging++.h"

AudioCircularQueue::AudioCircularQueue(int capability)
	 : m_capability(capability)
{

}

AudioCircularQueue::~AudioCircularQueue()
{
	while (m_queue.size())
	{
		m_queue.erase(begin(m_queue));
	}
}

void AudioCircularQueue::SetCapability(int capability)
{
	m_capability = capability;
}

size_t AudioCircularQueue::GetSize(bool lock)
{
	std::unique_lock<std::mutex> ulock(m_mutex, std::defer_lock);
	if (lock)
		ulock.lock();

	size_t size = 0;
	for (int i=0; i<m_queue.size(); i++)
	{
		size += m_queue[i].size();
	}
	size -= m_head;

	//elogd("[AudioCircularQueue::GetSize] this(0x%x) size(%d)", this, size);
	return size;
}

size_t AudioCircularQueue::Queue(std::vector<uint8_t>&& source, bool lock)
{
	//elogd("[AudioCircularQueue::Queue] this(0x%x) size(%d)", this, source.size());

	std::unique_lock<std::mutex> ulock(m_mutex, std::defer_lock);
	if (lock)
		ulock.lock();

	size_t size = 0;
	for (int i=0; i<m_queue.size(); i++)
	{
		size += m_queue[i].size();
	}
	size += source.size();

	if (size > m_capability)
	{
		size_t overflow = size - m_capability;
		Dequeue(nullptr, (int)overflow, false);
		eloge("[AudioCircularQueue::Queue] overflow(%d)", overflow);
	}
	
	m_queue.push_back(source);
	
	return m_queue[m_queue.size()-1].size();
}

size_t AudioCircularQueue::Queue(const uint8_t* pSource, int size)
{
	std::vector<uint8_t> new_buffer(size);
	memcpy(new_buffer.data(), pSource, size);
	return Queue(move(new_buffer), true);
}

size_t AudioCircularQueue::Dequeue(uint8_t* pTarget, int size, bool lock)
{
	//elogd("[AudioCircularQueue::Dequeue] this(0x%x) size(%d)", this, size);

	std::unique_lock<std::mutex> ulock(m_mutex, std::defer_lock);
	if (lock)
		ulock.lock();

	size_t total_read_size = 0;
	while (total_read_size < size)
	{
		size_t current_read_size = m_queue[0].size()-m_head;
		if (size >= total_read_size + current_read_size)
		{
			if (pTarget!=nullptr)
			{
				memcpy(&pTarget[total_read_size], &m_queue[0].data()[m_head], current_read_size);
			}
			m_head += current_read_size;
		}
		else
		{
			current_read_size = size - total_read_size;
			if (pTarget!=nullptr)
			{
				memcpy(&pTarget[total_read_size], &m_queue[0].data()[m_head], current_read_size);
			}
			m_head = current_read_size;
		}
		
		if (m_head >= m_queue[0].size())
		{
			m_queue.erase(begin(m_queue));
			m_head = 0;
		}

		total_read_size += current_read_size;
	}
	return total_read_size;
}