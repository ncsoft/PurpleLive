#pragma once


#include <stdint.h>
#include <string>
#include <inttypes.h>
#include <mutex>

using namespace std;

class CTransactionId
{
public:
	CTransactionId()
	{
		m_nTransactionId = 0;
	}

	~CTransactionId()
	{
	}

	uint64_t Gen()
	{
		std::lock_guard<std::mutex> guard(m_Mutex);
		return ++m_nTransactionId;
	}

	string GenStr()
	{
		std::lock_guard<std::mutex> guard(m_Mutex);
		++m_nTransactionId;
		snprintf(m_sBuf, sizeof(m_sBuf), "%" PRIu64, m_nTransactionId);
		return m_sBuf;
	}

protected:
	std::mutex m_Mutex;
	uint64_t m_nTransactionId;
	char m_sBuf[128];
};
