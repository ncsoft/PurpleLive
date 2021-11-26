#pragma once
#include "Singleton.h"
#include <QSettings>

#define HIDDEN_SETTINGS_APP_DEBUG				"hidden/app_debug"
#define HIDDEN_SETTINGS_WEB_DEBUG				"hidden/web_debug"


class HiddenSettings : public CommunityLiveCore::Singleton<HiddenSettings>
{
public:
	explicit HiddenSettings();
	virtual ~HiddenSettings();	

	static QSettings* GetSettings()
	{
		return Instance()->m_settings;
	}

	static constexpr char* hidden_settings_file_name = "HiddenSettings.ini";

	QString GetFilePath();
	void LoadFile();
	void SaveFile();
	void RemoveFile();

	static bool GetEnableAppDebug(bool default_value = false);
	static void SetEnableAppDebug(bool enable);

	static bool GetEnableWebDebug(bool default_value = false);
	static void SetEnableWebDebug(bool enable);

private:
	QSettings* m_settings = nullptr;
};