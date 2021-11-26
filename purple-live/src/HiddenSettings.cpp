#include "HiddenSettings.h"
#include "AppConfig.h"
#include "AppUtils.h"
#include "easylogging++.h"
#include <QCoreApplication>
#include <QFile>

HiddenSettings::HiddenSettings()
{
	LoadFile();
}

HiddenSettings::~HiddenSettings()
{
	if (m_settings!=nullptr)
	{
		delete m_settings;
		m_settings = nullptr;
	}
}

QString HiddenSettings::GetFilePath()
{
	return QCoreApplication::applicationDirPath() + "/" + hidden_settings_file_name;
}

void HiddenSettings::LoadFile()
{
	m_settings = new QSettings(GetFilePath(), QSettings::IniFormat);
	m_settings->setIniCodec("UTF-8");
}

void HiddenSettings::SaveFile()
{
	if (m_settings)
		m_settings->sync();
}

void HiddenSettings::RemoveFile()
{
	QFile::remove(GetFilePath());
}

bool HiddenSettings::GetEnableAppDebug(bool default_value)
{
	QVariant variant = GetSettings()->value(HIDDEN_SETTINGS_APP_DEBUG, default_value);
	return variant.toBool();
}

void HiddenSettings::SetEnableAppDebug(bool enable)
{
	GetSettings()->setValue(HIDDEN_SETTINGS_APP_DEBUG, enable);
	Instance()->SaveFile();
}

bool HiddenSettings::GetEnableWebDebug(bool default_value)
{
	QVariant variant = GetSettings()->value(HIDDEN_SETTINGS_WEB_DEBUG, default_value);
	return variant.toBool();
}

void HiddenSettings::SetEnableWebDebug(bool enable)
{
	GetSettings()->setValue(HIDDEN_SETTINGS_WEB_DEBUG, enable);
	Instance()->SaveFile();
}