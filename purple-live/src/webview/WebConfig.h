#pragma once

#include "lime-defines.h"
#include "VideoChatApp.h"

class WebConfig
{
public:
	static constexpr const char* domain_op = "https://***********";
	static constexpr const char* domain_sb = "https://***********";
	static constexpr const char* domain_rc = "https://***********";
	static constexpr const char* domain_live = "https://***********";

	static constexpr const char* theme_light = "light";
	static constexpr const char* theme_dark = "dark";
	static constexpr const char* theme_default = theme_dark;

	static QString GetWebServiceDomain(eServiceNetwork sn)
	{
		switch (sn) {
		case eSN_OP:
			return domain_op;
		case eSN_SB:
			return domain_sb;
		case eSN_RC:
			return domain_rc;
		case eSN_LIVE:
		default:
			return domain_live;
		}
	}

	static QString GetThemeString(eThemeType themeType)
	{
		switch (themeType)
		{
		case eThemeType::eTT_Light:
			return theme_light;
		case eThemeType::eTT_Purple:
			return theme_default;
		case eThemeType::eTT_Dark:
			return theme_dark;
		default:
			return theme_default;
		}
	}

	static QString GetCreateChattingUrl(eServiceNetwork sn, QString token, QString theme = theme_default)
	{
		//url 파라미터에 theme={theme}를 붙여서 호출하면 웹에 테마가 적용되며, 기본값은 light 입니다. (dark/light)
		return QString("%1/live/webview/createChat?limeToken=%2&theme=%3").arg(GetWebServiceDomain(sn), token, theme);
	}
};