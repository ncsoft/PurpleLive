#pragma once

#include <QWebEngineProfile>
#include <qwebengineurlrequestjob>
#include <QWebEngineUrlSchemeHandler>
#include "WebCommunicator.h"

class UrlSchemeHandler : public QWebEngineUrlSchemeHandler
{
    Q_OBJECT
public:
    explicit UrlSchemeHandler(QObject *parent = nullptr) {}

    void requestStarted(QWebEngineUrlRequestJob *job) override 
	{
		WebCommunicator::Instance()->FilterUriScheme(job->requestUrl());
	}
};