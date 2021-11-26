#pragma once

#include "WebConfig.h"
#include <QApplication>
#include <QWebEngineProfile>
#include <QWebEngineView>
#include <QWebEngineUrlRequestInterceptor>
#include <QDebug>
#include "VideoChatApp.h"
class WebviewUrlRequestInterceptor : public QWebEngineUrlRequestInterceptor
{
    Q_OBJECT
public:
	using QWebEngineUrlRequestInterceptor::QWebEngineUrlRequestInterceptor;
	
    void interceptRequest(QWebEngineUrlRequestInfo &info)
    {
/*
Accept-Language	로케일(언어)	ko-KR, en-US, zh-TW, zh-CN, ja-JP
X-LauncherId	Lime-Device-Id 전달 용도	
X-UserId		현재 로그인한 사용자의 NP UserId	
User-Agent		퍼플존용 User-Agent : PurpleLive/[version] (Windows)
*/
		info.setHttpHeader("Accept-Language", App()->GetLocalLanguage().toStdString().c_str());
		info.setHttpHeader("X-LauncherId", App()->GetDeviceId().c_str());
		info.setHttpHeader("X-UserId", "");
    }
};