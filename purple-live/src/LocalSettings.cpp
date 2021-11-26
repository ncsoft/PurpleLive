#include "LocalSettings.h"
#include "AppConfig.h"
#include "AppUtils.h"
#include "easylogging++.h"
#include <qapplication.h>
#include <string>
#include <qstandardpaths.h>

LocalSettings::LocalSettings()
{
	LoadFile();
}

LocalSettings::~LocalSettings()
{
	SaveFile();
	if (m_settings!=nullptr)
	{
		delete m_settings;
		m_settings = nullptr;
	}
	elogd("[LocalSettings::~LocalSettings] Singleton");
}

void LocalSettings::LoadFile()
{
	QString path = QStandardPaths::standardLocations(QStandardPaths::GenericConfigLocation)[0] + "/" + AppConfig::local_settings_path + "/" + AppConfig::local_settings_file_name;
	m_settings = new QSettings(path, QSettings::IniFormat);
	m_settings->setIniCodec("UTF-8");
}

void LocalSettings::SaveFile()
{
	if (m_settings)
		m_settings->sync();
}

QString LocalSettings::GetTheme(const QString& default_value)
{
	QVariant variant = GetSettings()->value(LOCAL_SETTINGS_NAME_THEME, default_value);
	return variant.toString();
}

void LocalSettings::SetTheme(const QString& value)
{
	GetSettings()->setValue(LOCAL_SETTINGS_NAME_THEME, value);
	Instance()->SaveFile();
}

QBitArray LocalSettings::GetGameMaskSettings(std::string game_code, QBitArray& defalut_list)
{
	std::string name = AppUtils::format(LOCAL_SETTINGS_NAME_GAME_MASK_SETTINGS, game_code.c_str());
	QVariant variant = GetSettings()->value(name.c_str(), defalut_list);

	// 가리기 항목 수 변경 시 보완
	if (variant.toBitArray().size() < defalut_list.size())
	{
		for (int i=0; i<variant.toBitArray().size(); i++)
		{
			defalut_list.setBit(i, variant.toBitArray().at(i));
		}
		return defalut_list;
	}

	return variant.toBitArray();
}

void LocalSettings::SetGameMaskSettings(std::string game_code, QBitArray& list)
{
	std::string name = AppUtils::format(LOCAL_SETTINGS_NAME_GAME_MASK_SETTINGS, game_code.c_str());
	GetSettings()->setValue(name.c_str(), list);
	Instance()->SaveFile();
}

void LocalSettings::SetGameMaskSettings(std::string game_code, int count, int index, bool enable)
{
	QBitArray settings = GetGameMaskSettings(game_code, QBitArray(count));
	settings.setBit(index, enable);
	SetGameMaskSettings(game_code, settings);
}

bool LocalSettings::GetEnableGameMask(bool default_value)
{
	std::string name = AppUtils::format(LOCAL_SETTINGS_NAME_GAME_MASK_ENABLE);
	QVariant variant = GetSettings()->value(name.c_str(), default_value);
	return variant.toBool();
}

void LocalSettings::SetEnableGameMask(bool enable)
{
	std::string name = AppUtils::format(LOCAL_SETTINGS_NAME_GAME_MASK_ENABLE);
	GetSettings()->setValue(name.c_str(), enable);
	Instance()->SaveFile();
}

QRect LocalSettings::GetWindowGeometry(std::string window_name, const QRect& default_rect)
{
	std::string name = AppUtils::format(LOCAL_SETTINGS_NAME_PREVIEW_GEOMETRY, window_name.c_str());
	QVariant variant = GetSettings()->value(name.c_str(), default_rect);
	return variant.toRect();
}

void LocalSettings::SetWindowGeometry(std::string window_name, const QRect& rect)
{
	std::string name = AppUtils::format(LOCAL_SETTINGS_NAME_PREVIEW_GEOMETRY, window_name.c_str());
	GetSettings()->setValue(name.c_str(), rect);
	Instance()->SaveFile();
}

bool LocalSettings::GetWindowMaximized(std::string window_name, bool& isMaximizedSize)
{
	std::string name = AppUtils::format(LOCAL_SETTINGS_NAME_WINDOW_MAXIMIZED, window_name.c_str());
	QVariant variant = GetSettings()->value(name.c_str(), isMaximizedSize);
	return variant.toBool();
}

void LocalSettings::SetWindowMaximized(std::string window_name, bool isMaximizedSize)
{
	std::string name = AppUtils::format(LOCAL_SETTINGS_NAME_WINDOW_MAXIMIZED, window_name.c_str());
	GetSettings()->setValue(name.c_str(), isMaximizedSize);
	Instance()->SaveFile();
}

QString LocalSettings::GetString(const QString& type, const QString& default_value)
{
	std::string name = AppUtils::format(LOCAL_SETTINGS_NAME_STRINGS, type.toStdString().c_str());
	QVariant variant = GetSettings()->value(name.c_str(), default_value);
	return variant.toString();
}

void LocalSettings::SetString(const QString& type, const QString& value)
{
	std::string name = AppUtils::format(LOCAL_SETTINGS_NAME_STRINGS, type.toStdString().c_str());
	GetSettings()->setValue(name.c_str(), value);
	Instance()->SaveFile();
}

QString LocalSettings::GetSelectedMicDeviceID(const QString& default_value)
{
	QVariant variant = GetSettings()->value(LOCAL_SETTINGS_NAME_MIC_ID, default_value);
	return variant.toString();
}

void LocalSettings::SetSelectedMicDeviceID(const QString& value)
{
	GetSettings()->setValue(LOCAL_SETTINGS_NAME_MIC_ID, value);
	Instance()->SaveFile();
}

bool LocalSettings::GetSelectedDefaultMicDevice(bool default_value)
{
	QVariant variant = GetSettings()->value(LOCAL_SETTINGS_NAME_MIC_DEFAULT, default_value);
	return variant.toBool();
}

void LocalSettings::SetSelectedDefaultMicDevice(bool value)
{
	GetSettings()->setValue(LOCAL_SETTINGS_NAME_MIC_DEFAULT, value);
	Instance()->SaveFile();
}

float LocalSettings::GetMicVolume(float default_value)
{
	QVariant variant = GetSettings()->value(LOCAL_SETTINGS_NAME_MIC_VOLUME, default_value);
	return variant.toFloat();
}

void LocalSettings::SetMicVolume(float value)
{
	GetSettings()->setValue(LOCAL_SETTINGS_NAME_MIC_VOLUME, value);
	Instance()->SaveFile();
}

bool LocalSettings::GetMicMute(bool default_value)
{
	QVariant variant = GetSettings()->value(LOCAL_SETTINGS_NAME_MIC_MUTE, default_value);
	return variant.toBool();
}

void LocalSettings::SetMicMute(bool value)
{
	GetSettings()->setValue(LOCAL_SETTINGS_NAME_MIC_MUTE, value);
	Instance()->SaveFile();
}

QString LocalSettings::GetLanguage(const QString& default_value)
{
	QVariant variant = GetSettings()->value(LOCAL_SETTINGS_NAME_LANGUAGE, default_value);
	return variant.toString();
}

void LocalSettings::SetLanguage(QString& value)
{
	GetSettings()->setValue(LOCAL_SETTINGS_NAME_LANGUAGE, value);
	Instance()->SaveFile();
}