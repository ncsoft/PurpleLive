#include "VideoChatApp.h"
#include "VideoChatArguments.h"
#include "AppExcuteHelper.h"
#include "AppUtils.h"
#include "easylogging++.h"

#ifdef _DEBUG
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#endif

int main(int argc, char *argv[])
{
	DumpUtils::initialize_crach_handler();

	AppUtils::InitLog();

	// set dpi awareness
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
	QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

	AppExcuteHelper aeh;

	if (aeh.IsEnableAppDestroyEvent())
	{
		return 0;
	}

	if (!aeh.CreateAppExcuteEvent())
	{
		if (aeh.IsForceExcute(argc, argv))
		{
			aeh.ProcessAppExcuteEvent(true);
		}
		else
		{
			aeh.ProcessAppExcuteEvent(false);
			return 0;
		}
	}

	VideoChatApp app(argc, argv);

	aeh.StartWaitAppExcuteEvent(&app);

	if (app.Init() == false)
		return 0;

	// active main event loop
	int res = app.exec();

	aeh.StopWaitAppExcuteEvent();
	if (aeh.IsRetryExcution())
	{
		aeh.RetryExcution();
	}

    return res;
}
