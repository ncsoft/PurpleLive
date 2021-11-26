#include "AppExcuteHelper.h"
#include "AppConfig.h"
#include <Windows.h>
#include <qthread.h>
#include <qbuffer.h>
#include "easylogging++.h"
#include "AppUtils.h"

AppEventThread::AppEventThread(AppExcuteHelper* appExcpteHelper, bool bAppExcuteEvent)
	: m_AppExcpteHelper(appExcpteHelper), m_bAppExcuteEvent(bAppExcuteEvent)
{
	
}

void AppEventThread::run()
{
	if (m_bAppExcuteEvent)
	{
		m_AppExcpteHelper->ProcessWaitExcuteEvent();
	}
	else
	{
		m_AppExcpteHelper->ProcessWaitDestoryEvent();
	}
}

AppExcuteHelper::AppExcuteHelper()
{
	
}

AppExcuteHelper::~AppExcuteHelper()
{
	if (g_pAppExcuteEventThread)
	{
		StopWaitAppExcuteEvent();
		delete g_pAppExcuteEventThread;
		g_pAppExcuteEventThread = nullptr;
	}

	if (g_pAppDestroyEventThread)
	{
		StopWaitAppDestroyEvent();
		delete g_pAppDestroyEventThread;
		g_pAppDestroyEventThread = nullptr;
	}

	LOG(DEBUG) << "[AppExcuteHelper::~AppExcuteHelper]";
}

bool AppExcuteHelper::IsForceExcute(int &argc, char **argv)
{
	return VideoChatArguments::IsForceExcute(argc, argv);
}

void AppExcuteHelper::ProcessWaitExcuteEvent()
{
	LOG(DEBUG) << "[AppExcuteHelper::ProcessWaitExcuteEvent] start";
	while (WaitForSingleObject(GetAppEventExcuteHandle(), INFINITE)==WAIT_OBJECT_0)
	{
		if (GetApp()==nullptr)
			break;

		if (IsAppDestroying())
			break;

		if (ReadSharedMemory())
		{
			GetApp()->OnAppEvent(eAppEventType::eAET_RetryExcution);
			break;
		}
		else
		{
			GetApp()->OnAppEvent(eAppEventType::eAET_BringToFront);
		}
	}
	LOG(DEBUG) << "[AppExcuteHelper::ProcessWaitExcuteEvent] finish";
}

void AppExcuteHelper::ProcessWaitDestoryEvent()
{
	LOG(DEBUG) << "[AppExcuteHelper::ProcessWaitDestoryEvent] start";
	WaitForSingleObject(GetAppEventDestroyHandle(), 10000);
	LOG(DEBUG) << "[AppExcuteHelper::ProcessWaitDestoryEvent] finish";
}

bool AppExcuteHelper::IsEnableAppDestroyEvent()
{
	bool res = !CreateAppDestroyEvent();
	CloseHandle(m_hAppDestroyEvent);
	return res;
}

bool AppExcuteHelper::CreateAppExcuteEvent()
{
    m_hAppExcuteEvent = CreateEventA(NULL, FALSE, FALSE, AppConfig::APP_EXCUTE_EVENT_ID); // or CreateMutex 
    if (ERROR_ALREADY_EXISTS == GetLastError()) { 
		LOG(DEBUG) << "[AppExcuteHelper::CreateAppEvent] CreateEventA(APP_EXCUTE_EVENT_ID) ERROR_ALREADY_EXISTS";
        return false;
    }
	return true;
}

bool AppExcuteHelper::CreateAppDestroyEvent()
{
    m_hAppDestroyEvent = CreateEventA(NULL, FALSE, FALSE, AppConfig::APP_DESTROY_EVENT_ID); // or CreateMutex 
    if (ERROR_ALREADY_EXISTS == GetLastError()) { 
		LOG(DEBUG) << "[AppExcuteHelper::CreateAppEvent] CreateEventA(APP_DESTROY_EVENT_ID) ERROR_ALREADY_EXISTS";
        return false;
    }
	return true;
}



void AppExcuteHelper::ProcessAppExcuteEvent(bool isForceExcute)
{
	if (isForceExcute)
	{
		WriteSharedMemory();
		ProcessWaitDestoryEvent();
	}
	else
	{
		SetEvent(m_hAppDestroyEvent);
	}
}

void AppExcuteHelper::StartWaitAppExcuteEvent(VideoChatApp* pApp)
{
	m_pApp = pApp;
	g_pAppExcuteEventThread = new AppEventThread(this, true);
	g_pAppExcuteEventThread->start();
}

void AppExcuteHelper::StartWaitAppDestroyEvent()
{
	g_pAppDestroyEventThread = new AppEventThread(this, false);
	g_pAppDestroyEventThread->start();
}

void AppExcuteHelper::StopWaitAppExcuteEvent()
{
	if (m_hAppExcuteEvent)
	{
		LOG(DEBUG) << "[AppExcuteHelper::StopWaitAppExcuteEvent] start";
		m_bAppDestorying = true;
		SetEvent(m_hAppExcuteEvent);

		if (g_pAppExcuteEventThread)
		{
			while (g_pAppExcuteEventThread->isRunning())
				Sleep(10);

			g_pAppExcuteEventThread->exit();
		}

		CloseHandle(m_hAppExcuteEvent);
		m_hAppExcuteEvent = nullptr;
		LOG(DEBUG) << "[AppExcuteHelper::StopWaitAppExcuteEvent] finish";
	}
}

void AppExcuteHelper::StopWaitAppDestroyEvent()
{
	if (m_hAppDestroyEvent)
	{
		LOG(DEBUG) << "[AppExcuteHelper::StopWaitAppDestroyEvent] start";
		SetEvent(m_hAppDestroyEvent);

		if (g_pAppDestroyEventThread)
		{
			while (g_pAppDestroyEventThread->isRunning())
				Sleep(10);

			g_pAppDestroyEventThread->exit();
		}

		CloseHandle(m_hAppDestroyEvent);
		m_hAppDestroyEvent = nullptr;
		LOG(DEBUG) << "[AppExcuteHelper::StopWaitAppDestroyEvent] finish";
	}
}

bool AppExcuteHelper::ReadSharedMemory()
{
	QSharedMemory shm{AppConfig::APP_SHM_ID};
	shm.setNativeKey(AppConfig::APP_SHM_ID);
	shm.setKey(AppConfig::APP_SHM_ID);

	if (!shm.attach())
	{
		QString error = shm.errorString();
		LOG(DEBUG) << AppUtils::format("[AppExcuteHelper::ReadSharedMemory] attach failed (%s)", shm.errorString().toStdString().c_str());
		return false;
	}

	shm.lock();
	{
		QBuffer buffer;	
		buffer.setData((char*)shm.constData(), shm.size());
		buffer.open(QBuffer::ReadOnly);
		m_strCommand = buffer.buffer();
		m_bRetryExcution = true;
	}
	shm.unlock();
	shm.detach();
	return true;
}

void AppExcuteHelper::WriteSharedMemory(int waitTime)
{
	//static QSharedMemory shm{AppConfig::APP_SHM_ID};
	QSharedMemory shm{AppConfig::APP_SHM_ID};
	shm.setNativeKey(AppConfig::APP_SHM_ID);
	shm.setKey(AppConfig::APP_SHM_ID);

	if (shm.isAttached())
    {
		shm.detach();
    }

	string command{GetCommandLineA()};
	shm.create(command.size());
    shm.lock();
	{
		char *to = (char*)shm.data();
		const char *from = (const char*)command.data();
		memcpy(to, from, min(shm.size(), command.size()));
	}
	shm.unlock();

	LOG(DEBUG) << AppUtils::format("[AppExcuteHelper::WriteSharedMemory] command(%d)", command);

	SetEvent(m_hAppExcuteEvent);

	Sleep(waitTime);

	LOG(DEBUG) << AppUtils::format("[AppExcuteHelper::WriteSharedMemory] wait(%d)", waitTime);
}

void AppExcuteHelper::RetryExcution()
{
	LOG(DEBUG) << AppUtils::format("[AppExcuteHelper::RetryExcution]");

	if (CreateAppDestroyEvent())
	{
		SetEvent(m_hAppDestroyEvent);
	}
}

void AppExcuteHelper::AppExcute()
{
	LOG(DEBUG) << AppUtils::format("[AppExcuteHelper::AppExcute] command(%s)", m_strCommand.c_str());

	string path = VideoChatArguments::GetProgramPath(m_strCommand.c_str());
	string arguments = VideoChatArguments::GetArguments(m_strCommand.c_str(), path);

	SHELLEXECUTEINFOA ShExecInfo;
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask = NULL;
    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = NULL;
    ShExecInfo.lpFile = path.c_str();
    ShExecInfo.lpParameters = arguments.c_str();
    ShExecInfo.lpDirectory = NULL;
    ShExecInfo.nShow = SW_SHOW;
    ShExecInfo.hInstApp = NULL;
	ShellExecuteExA(&ShExecInfo);
}
