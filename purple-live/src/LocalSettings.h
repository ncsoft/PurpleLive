#pragma once
#include "Singleton.h"
#include <string>
#include <qstring.h>
#include <qsettings.h>
#include <bitset>
#include <qrect.h>
#include <qbitarray.h>

#define LOCAL_SETTINGS_NAME_THEME				"general/theme"
#define LOCAL_SETTINGS_NAME_PREVIEW_GEOMETRY	"window_geometry/%s"
#define LOCAL_SETTINGS_NAME_GAME_MASK_SETTINGS	"game_mask_settings/%s"
#define LOCAL_SETTINGS_NAME_GAME_MASK_ENABLE	"game_mask_settings/enable_mask"
#define LOCAL_SETTINGS_NAME_MIC_ID				"audio_settings/mic_id"
#define LOCAL_SETTINGS_NAME_MIC_DEFAULT			"audio_settings/mic_default"
#define LOCAL_SETTINGS_NAME_MIC_VOLUME			"audio_settings/mic_volume"
#define LOCAL_SETTINGS_NAME_MIC_MUTE			"audio_settings/mic_mute"
#define LOCAL_SETTINGS_NAME_STRINGS				"strings/%s"
#define LOCAL_SETTINGS_NAME_WINDOW_MAXIMIZED	"window_maximized/%s"
#define LOCAL_SETTINGS_NAME_LANGUAGE			"general/language"

class LocalSettings : public CommunityLiveCore::Singleton<LocalSettings>
{
public:
	explicit LocalSettings();
	virtual ~LocalSettings();	

	static QSettings* GetSettings()
	{
		return Instance()->m_settings;
	}

	void LoadFile();
	void SaveFile();

	static constexpr bool DEFAULT_ENABLE_GAME_MASK = false;
	
	static QString GetTheme(const QString& default_value);
	static void SetTheme(const QString& value);

	static QBitArray GetGameMaskSettings(std::string game_code, QBitArray& defalut_list);
	static void SetGameMaskSettings(std::string game_code, QBitArray& list);
	static void SetGameMaskSettings(std::string game_code, int count, int index, bool enable);

	static bool GetEnableGameMask(bool default_value);
	static void SetEnableGameMask(bool enable);

	static QRect GetWindowGeometry(std::string window_name, const QRect& default_rect);
	static void SetWindowGeometry(std::string window_name, const QRect& rect);
	static bool GetWindowMaximized(std::string window_name, bool& isMaximizedSize);
	static void SetWindowMaximized(std::string window_name, bool isMaximizedSize);

	static QString GetString(const QString& type, const QString& default_value);
	static void SetString(const QString& type, const QString& value);
	
	static QString GetSelectedMicDeviceID(const QString& default_value);
	static void SetSelectedMicDeviceID(const QString& value);

	static bool GetSelectedDefaultMicDevice(bool default_value);
	static void SetSelectedDefaultMicDevice(bool value);

	static float GetMicVolume(float default_value);
	static void SetMicVolume(float vaule);

	static bool GetMicMute(bool default_value);
	static void SetMicMute(bool value);

	static QString GetLanguage(const QString& default_value);
	static void SetLanguage(QString& value);

private:
	QSettings* m_settings = nullptr;
};