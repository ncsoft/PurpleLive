#include "VideoChatApp.h"

#include <qdir.h>
#include <qfileinfo.h>		// for check qss file
#include <qfontdatabase.h>

#include "VideoChatMessageBox.h"
#include "LimeLogin.hpp"
#include "MAPManager.h"
#include "AppConfig.h"
#include "AppUtils.h"
#include "AppString.h"
#include "LocalSettings.h"
#include "MaskingManager.h"
#include "easylogging++.h"

#pragma comment(lib, "version.lib")

VideoChatApp::VideoChatApp(int &argc, char **argv)
	: QApplication(argc, argv)
{
	LOG(DEBUG) << AppUtils::format("[VideoChatArguments] command(%s)", StringUtils::GetStringFromUtf8(GetCommandLineW()).c_str());

	m_Arguments.Parse(arguments());
}

VideoChatApp::~VideoChatApp()
{
	MAPManager::Instance()->LogEnd();
	delete m_mainWindow.data();
}

bool VideoChatApp::Init()
{
	LOG(DEBUG) << format_string("[VideoChatApp::Init]");
	
#if 1 // TODO : 임시로 다크테마로 지정. 테마 변경 기능 적용 시 제거해야 함. 
	if (!SetTheme(AppConfig::theme_dark))
		return false;
#else
	if (!SetTheme(LocalSettings::GetTheme(AppConfig::default_theme).toStdString().c_str()))
		return false;
#endif

	LoadFont();

	m_mainWindow = new MainWindow(nullptr);

	if (!(m_mainWindow->Init()))
		return false;

	m_pPurpleWSClient = std::make_unique<PurpleWebsocketClient>(std::atoi(GetAgentPort().c_str()), GetInstanceName().c_str());

	return true;
}

eThemeType VideoChatApp::GetTheme()
{
	return m_themeType;
}

bool VideoChatApp::SetTheme(eThemeType themeType)
{
	m_themeType = themeType;
	switch (themeType)
	{
		case eThemeType::eTT_Light:
			return SetTheme(AppConfig::theme_light);
		case eThemeType::eTT_Purple:
			return SetTheme(AppConfig::theme_purple);
		case eThemeType::eTT_Dark:
			return SetTheme(AppConfig::theme_dark);
		default:
			return false;
	}
	return false;
}

bool VideoChatApp::SetTheme(const char* theme)
{
	if (std::string(theme) == AppConfig::theme_light)
		m_themeType = eThemeType::eTT_Light;
	else if (std::string(theme) == AppConfig::theme_purple)
		m_themeType = eThemeType::eTT_Purple;
	else if (std::string(theme) == AppConfig::theme_dark)
		m_themeType = eThemeType::eTT_Dark;
	else
		return false;

	if (SetQssTheme(theme))
	{
		LocalSettings::SetTheme(theme);
		return true;
	}
	return false;
}

bool VideoChatApp::VerifyLanguageCode(QString& language)
{
	if (language == LanguageCode::ko_KR)
		return true;	
	if (language == LanguageCode::en_US)
		return true;
	if (language == LanguageCode::zh_TW)
		return true;
	if (language == LanguageCode::zh_CN)
		return true;
	if (language == LanguageCode::de_DE)
		return true;
	if (language == LanguageCode::es_ES)
		return true;
	if (language == LanguageCode::fr_FR)
		return true;
	if (language == LanguageCode::id_ID)
		return true;
	if (language == LanguageCode::ja_JP)
		return true;
	if (language == LanguageCode::pl_PL)
		return true;
	if (language == LanguageCode::ru_RU)
		return true;
	if (language == LanguageCode::th_TH)
		return true;
	if (language == LanguageCode::ar_SA)
		return true;
	if (language == LanguageCode::vi_VN)
		return true;
	if (language == LanguageCode::tr_TR)
		return true;
	
	LOG(ERROR) << format_string("[VideoChatApp::VerifyLanguageCode] invalid language code(%s)", language.toStdString().c_str());

	return false;
}

bool VideoChatApp::SetLocalLanguage(QString language)
{
	LOG(INFO) << format_string("[VideoChatApp::SetLocalLanguage] language(%s)", language.toStdString().c_str());

	if (VerifyLanguageCode(language))
	{
		LocalSettings::SetLanguage(language);
		AppString::SetLanguage(language);
		MainWindow::Get()->OnLanguageChanged();

		return true;
	}
	return false;
}

QString VideoChatApp::GetLocalLanguage()
{
	QString language = LocalSettings::GetLanguage(QString::fromStdString(AppConfig::default_language));
	if (!VerifyLanguageCode(language))
	{
		language = QString::fromStdString(AppConfig::default_language);
	}
	return language;
}

void VideoChatApp::SendStreamStatusToAgent(bool start)
{
	m_pPurpleWSClient->SendStreamStatus(start);
}

void VideoChatApp::SendScreenUnlockToAgent()
{
	m_pPurpleWSClient->SendScreenUnlock();
}

std::string VideoChatApp::GetOriginGameCode()
{
	std::string game_code = m_Arguments.m_gameCode;
	if (StringUtils::StartWith(game_code, GameCode::lms))
		return GameCode::lms;
	else if (StringUtils::StartWith(game_code, GameCode::l2m))
		return GameCode::l2m;
	else if (StringUtils::StartWith(game_code, GameCode::bns2))
		return GameCode::bns2;
	else if (StringUtils::StartWith(game_code, GameCode::tricksterm))
		return GameCode::tricksterm;
	else if (StringUtils::StartWith(game_code, GameCode::h3))
		return GameCode::h3;
	return game_code;
}

QString VideoChatApp::GetGuildLabel()
{
	return AppString::GetQString(AppUtils::format("%s/guild_label", GetOriginGameCode().c_str()).c_str());
}

void VideoChatApp::SetGameHwnd(HWND hwnd)
{
	m_Arguments.m_gameHwnd = AppUtils::format("%08x", (void*)hwnd);
}

void VideoChatApp::OnAppEvent(eAppEventType type)
{
	LOG(INFO) << format_string("[VideoChatApp::OnAppEvent] type(%d)", type);

	AppUtils::DispatchToMainThread([type]
	{
		switch (type)
		{
			case eAppEventType::eAET_BringToFront:
				MainWindow::Get()->BringToFront();
				break;
			case eAppEventType::eAET_RetryExcution:
				MainWindow::Get()->BringToFront();
				MainWindow::Get()->RetryExcution();
				break;
			default:
				break;
		}
	});
}

string VideoChatApp::GetAppVersion()
{
	TCHAR szFilename[MAX_PATH + 1] = { 0 };
	if (GetModuleFileName(NULL, szFilename, MAX_PATH) == 0)
	{
		return false;
	}

	// allocate a block of memory for the version info
	DWORD dummy;
	DWORD dwSize = GetFileVersionInfoSize(szFilename, &dummy);
	if (dwSize == 0)
	{
		return false;
	}
	std::vector<BYTE> data(dwSize);

	// load the version info
	if (!GetFileVersionInfo(szFilename, NULL, dwSize, &data[0]))
	{
		return false;
	}

	UINT                uiVerLen = 0;
	VS_FIXEDFILEINFO*   pFixedInfo = 0;     // pointer to fixed file info structure
	// get the fixed file info (language-independent) 
	if (VerQueryValue(&data[0], TEXT("\\"), (void**)&pFixedInfo, (UINT *)&uiVerLen) == 0)
	{
		return false;
	}

	string version = AppUtils::format("%u.%u.%u.%u",
		HIWORD(pFixedInfo->dwFileVersionMS),
		LOWORD(pFixedInfo->dwFileVersionMS),
		HIWORD(pFixedInfo->dwFileVersionLS),
		LOWORD(pFixedInfo->dwFileVersionLS));

	return version;
}

HWND VideoChatApp::GetGameHwnd()
{
	string strHwnd16 = format_string("0x%s", m_Arguments.m_gameHwnd.c_str());
	return reinterpret_cast<HWND>(strtoll(strHwnd16.c_str(), NULL, 0));
}

bool VideoChatApp::SetQssTheme(string _theme)
{
	string theme = _theme;

	QString path = QApplication::applicationDirPath() + "/data/theme/" + QString::fromStdString(theme) + ".qss";
	if (AppUtils::fileExists(path)) {
		// qss found
	} else {
		// qss does not exist
		return false;
	}
	QString mpath = QString("file:///") + path;
	setPalette(palette());
	setStyleSheet(mpath);

	emit StyleChanged();

	return true;
}

void VideoChatApp::LoadFont()
{
	QDir fontsDir(QApplication::applicationDirPath() + "/data/fonts/");
	QFileInfoList entries = fontsDir.entryInfoList();
	QString family;

	foreach (QFileInfo fileInfo, entries)
	{
		QString filePath = QApplication::applicationDirPath() + "/data/fonts/" + fileInfo.fileName();
		if (!AppUtils::fileExists(filePath)) {
			// font files are does not exist
			continue;
		}
		int id = QFontDatabase::addApplicationFont(filePath);
		family = QFontDatabase::applicationFontFamilies(id).at(0);
	}

	QFont default_font(family);
	QApplication::setFont(default_font);
}