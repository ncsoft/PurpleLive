#pragma once

#include <QApplication>
#include <QPointer>
#include "mainwindow.h"
#include "AppConfig.h"
#include "lime-defines.h"
#include "QTWebStompClient.h"
#include "LimeCommunicator.h"
#include "PurpleWebsocketClient.h"
#include "VideoChatArguments.h"

enum eThemeType {
	eTT_Light,
	eTT_Purple,
	eTT_Dark,
};

enum eAppEventType {
	eAET_None,
	eAET_BringToFront,
	eAET_RetryExcution,
};

class VideoChatApp : public QApplication
{
	Q_OBJECT

public:
	VideoChatApp(int &argc, char **argv);
	~VideoChatApp();

	bool				Init();

	eThemeType			GetTheme();
	bool				SetTheme(eThemeType themeType);
	bool				SetTheme(const char* theme);

	bool				VerifyLanguageCode(QString& language);
	bool				SetLocalLanguage(QString language);
	QString				GetLocalLanguage();

	void				SendStreamStatusToAgent(bool start);
	void				SendScreenUnlockToAgent();

	inline QMainWindow*	GetMainWindow() const { return m_mainWindow.data(); }
	void                OnAppEvent(eAppEventType type);

	std::string			GetAppVersion();
	eServiceNetwork		GetServiceNetwork() { return m_Arguments.m_serviceNetwork; }
	std::string			GetCharacterName() { return m_Arguments.m_characterName; }
	std::string			GetGameCode() { return m_Arguments.m_gameCode; }
	std::string			GetCharacterCode() { return m_Arguments.m_characterCode; }
	std::string			GetOriginGameCode();
	QString				GetGuildLabel();
	std::string			GetRoomID() { return m_Arguments.m_roomId; }
	std::string			GetToken() { return m_Arguments.m_token; }
	std::string			GetAuthnToken() { return m_Arguments.m_authn_token; }
	std::string			GetDeviceId() { return m_Arguments.m_deviceId;  }
	std::string			GetGameHwndString() { return m_Arguments.m_gameHwnd;  }
	HWND				GetGameHwnd();
	void				SetGameHwnd(HWND hwnd);
	std::string			GetGameId() { return m_Arguments.m_gameId; }
	std::string			GetAgentPort() { return m_Arguments.m_agentPort;  }
	std::string			GetInstanceName() { return m_Arguments.m_instanceName;  }
	std::string			GetGroupId() { return m_Arguments.m_groupId; }
	std::string			GetChannelId() { return m_Arguments.m_channelId; }
	std::string			GetArgLanguage() { return m_Arguments.m_language; }
	bool				GetForceExcute() { return m_Arguments.m_forceExcute; }
	void				SetCharacterName(std::string _s) { m_Arguments.m_characterName = _s; }
	bool				GetStandAlone() { return m_Arguments.m_standAlone; }

private:
	QPointer<MainWindow>	m_mainWindow;
	VideoChatArguments		m_Arguments;
	eThemeType				m_themeType;

	std::unique_ptr<PurpleWebsocketClient>	m_pPurpleWSClient;
	
	bool	SetQssTheme(string _theme);
	void	LoadFont();

signals:
	void StyleChanged();
};

inline VideoChatApp* App()
{
	return static_cast<VideoChatApp*>(qApp);
}
