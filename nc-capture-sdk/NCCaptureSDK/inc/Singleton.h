#pragma once

namespace NCCaptureSDK
{

template < typename T >
class Singleton
{
protected:
	Singleton()
	{

	}
	virtual ~Singleton()
	{

	}

public:
	static T * Instance()
	{
		if (m_pInstance == nullptr) {
			m_pInstance = new T;
			atexit(DestroyInstance);
		}
		return m_pInstance;
	};

	static void DestroyInstance()
	{
		if (m_pInstance)
		{
			delete m_pInstance;
			m_pInstance = nullptr;
		}
	};

private:
	static T * m_pInstance;
};

template <typename T> T * Singleton<T>::m_pInstance = 0;

}