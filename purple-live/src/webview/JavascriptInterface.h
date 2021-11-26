#pragma once
#include "Singleton.h"
#include "WebCommunicator.h"
#include "easylogging++.h"

class PurpleliveJavascriptInterface: public QObject
{
    Q_OBJECT
public:

    Q_INVOKABLE void on_loaded() const
    {
        elogi("[PurpleliveJavascriptInterface::on_loaded]");
		WebCommunicator::Instance()->SetJavascriptLoaded(true);
    }

	Q_INVOKABLE QString command(const QString& type, const QString& data) const
    {
		elogi("[PurpleliveJavascriptInterface::request_command] type(%s) data(%s)", type.toStdString().c_str(), data.toStdString().c_str());
        return WebCommunicator::Instance()->WebToAppCommand(type, data);
    }

	Q_INVOKABLE void report_channel_creation(int result, const QString& data) const
    {
        elogi("[PurpleliveJavascriptInterface::report_channel_creation] result(%d) data(%s)", result, data.toStdString().c_str());
        WebCommunicator::Instance()->ReportChannelCreation(result, data);
    }
};
