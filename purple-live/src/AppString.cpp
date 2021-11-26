#include "AppString.h"
#include "AppConfig.h"
#include "AppUtils.h"
#include "easylogging++.h"
#include "MainWindow.h"
#include "LocalSettings.h"

#include <qapplication.h>
#include <string>

AppString::AppString()
{
}

AppString::~AppString()
{
	elogd("[AppString::~AppString] Singleton");
}

bool AppString::SetLanguageimpl(QString language)
{
	if (language != "" || language != m_filePath)
	{
		m_filePath = language;

		QString path = QApplication::applicationDirPath() + "/data/local/" + m_filePath + ".ini";
		if (!AppUtils::fileExists(path)) {
			path = QApplication::applicationDirPath() + "/data/local/" + AppConfig::default_language + ".ini";
		}

		if (m_settings)
		{
			m_settings.reset(new QSettings(path, QSettings::IniFormat));
		}
		else
		{
			m_settings = std::make_shared<QSettings>(path, QSettings::IniFormat);
		}

		m_settings->setIniCodec("UTF-8");
		MainWindow::Get()->UpdateLanguageString();

		return true;
	}

	return false;
}