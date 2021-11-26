#pragma once

#include "VideoChatApp.h"
#include <qthread.h>
#include <qsharedmemory.h>

class AppExcuteHelper;

class AppEventThread : public QThread
{
    Q_OBJECT
public:
    AppEventThread(AppExcuteHelper* appExcpteHelper, bool bAppExcuteEvent);

protected:
    void run() override;

private:
	bool m_bAppExcuteEvent;
	AppExcuteHelper* m_AppExcpteHelper;
};

class AppExcuteHelper
{
public:
	AppExcuteHelper();
	virtual ~AppExcuteHelper();

	void ProcessWaitExcuteEvent();
	void ProcessWaitDestoryEvent();
	
	bool IsForceExcute(int &argc, char **argv);
	bool IsEnableAppDestroyEvent();
	bool CreateAppExcuteEvent();
	bool CreateAppDestroyEvent();
	void ProcessAppExcuteEvent(bool isForceExcute);

	void StartWaitAppExcuteEvent(VideoChatApp* pApp);
	void StartWaitAppDestroyEvent();
	void StopWaitAppExcuteEvent();
	void StopWaitAppDestroyEvent();

	bool ReadSharedMemory();
	void WriteSharedMemory(int waitTime = 100);

	void RetryExcution();
	void AppExcute();

	VideoChatApp* GetApp() { return m_pApp; }
	HANDLE GetAppEventExcuteHandle() { return m_hAppExcuteEvent; }
	HANDLE GetAppEventDestroyHandle() { return m_hAppDestroyEvent; }
	bool IsRetryExcution() { return m_bRetryExcution; }
	bool IsAppDestroying() { return m_bAppDestorying; }

private:
	VideoChatApp*	m_pApp = nullptr;
	HANDLE			m_hAppExcuteEvent = nullptr;
	HANDLE			m_hAppDestroyEvent = nullptr;
	bool			m_bAppDestorying = false;
	bool			m_bRetryExcution = false;
	std::string		m_strCommand;

	AppEventThread*	g_pAppExcuteEventThread = nullptr;
	AppEventThread*	g_pAppDestroyEventThread = nullptr;
};