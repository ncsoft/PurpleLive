#pragma once
#include "Singleton.h"
#include <string>
#include <qstring.h>
#include <qsettings.h>

class AppString : public CommunityLiveCore::Singleton<AppString>
{
public:
	explicit AppString();
	virtual ~AppString();	

	static std::shared_ptr<QSettings> GetSettings()
	{
		return Instance()->m_settings;
	}

	static bool SetLanguage(QString language)
	{
		return AppString::Instance()->SetLanguageimpl(language);
	}

	static QString GetQString(const char* name)
	{
		if (GetSettings()==nullptr)
			return "";

		return GetSettings()->value(name).toString();
	}

	static std::string GetString(const char* name)
	{
		return GetQString(name).toStdString();
	}

	bool SetLanguageimpl(QString language);

private:
	QString m_filePath{};
	std::shared_ptr<QSettings> m_settings = nullptr;
};