#include "WebCommunicator.h"
#include "WebviewConfig.h"
#include "WebviewControl.h"
#include "AppUtils.h"
#include "UrlSchemeHandler.h"
#include "JavascriptInterface.h"
#include "VideoChatMessageBox.h"

WebCommunicator::WebCommunicator()
{

}

WebCommunicator::~WebCommunicator()
{
	elogd("[WebCommunicator::~WebCommunicator] Singleton");
}

QString javascript_to_inject = QStringLiteral(
	R"DELIM(

		function a2w_alert(message) {
			alert(message);
		}

	)DELIM");

void WebCommunicator::SetCharactorId(const QString& charactor_id)
{
	m_character_id = charactor_id;
}

void WebCommunicator::SetCharactorInfo(const QString& charactor_info)
{
	m_character_info = charactor_info;
}

void WebCommunicator::SetWebView(WebView* webview)
{
	m_webview = webview;
}

void WebCommunicator::SetWebChannel(WebView* webview)
{
	static QWebChannel channel;
	static PurpleliveJavascriptInterface js_interface;
	webview->page()->setWebChannel(&channel);
	channel.registerObject(QString(js_interface.metaObject()->className()), &js_interface);
	//RunJavascript(javascript_to_inject);
}

void WebCommunicator::InitUriScheme(QWebEngineProfile *profile)
{
	static UrlSchemeHandler handler;
	if (profile)
		profile->installUrlSchemeHandler(APP_URI_SCHEME, &handler);
}

bool WebCommunicator::FilterUriScheme(const QUrl& url)
{
	if (url.scheme()==APP_URI_SCHEME)
	{
		if (url.host()=="close")
		{
			ShowPopupMessage(QString::fromLocal8Bit("URI Scheme을 통해서 close 요청을 받았습니다."));
			WebviewControl::Instance()->SetShowWebviewWidget(false);
		}

		return true;
	}
	return false;
}

bool WebCommunicator::GetJavascriptLoaded()
{
	return m_javascript_loaded;
}

void WebCommunicator::SetJavascriptLoaded(bool loaded)
{
	m_javascript_loaded = loaded;
}

void WebCommunicator::RunJavascript(const QString& javascript, const QWebEngineCallback<const QVariant &> &result_callback)
{
	m_webview->page()->runJavaScript(javascript, result_callback);
}

void WebCommunicator::AppToWebCommand(const QString& type, const QString& data, const QWebEngineCallback<const QVariant&> &result_callback)
{
	elogd("[WebCommunicator::AppToWebCommand] type(%s) data(%s)", type.toStdString().c_str(), data.toStdString().c_str());

	if (QString::compare(type, a2w_command::get_chatting_channel_id, Qt::CaseInsensitive)==0)
	{
		std::string javascript = AppUtils::format("a2w_command('%s', '');", type.toStdString().c_str());
		RunJavascript(javascript.c_str(), [this](const QVariant& result) {
			ShowPopupMessage(QString::fromLocal8Bit("웹으로 요청하여 리턴받은 채널 ID : ") + result.toString()); 
		});
	}
}

QString WebCommunicator::WebToAppCommand(const QString& type, const QString& data)
{
	elogd("[WebCommunicator::WebToAppCommand] type(%s) data(%s)", type.toStdString().c_str(), data.toStdString().c_str());

	if (QString::compare(type, w2a_command::log, Qt::CaseInsensitive)==0)
	{
		elogi("[WebCommunicator::WebToAppCommand] type(%s) data(%s)", type.toStdString().c_str(), data.toStdString().c_str());
	}
	else if (QString::compare(type, w2a_command::get_character_id, Qt::CaseInsensitive)==0)
	{
		//ShowPopupMessage(QString::fromLocal8Bit("[웹->앱] get_character_id : ") + m_character_id); 
		elogd("[WebCommunicator::WebToAppCommand] return character_id(%s)", m_character_id.toStdString().c_str());
		return m_character_id;
	}
	else if (QString::compare(type, w2a_command::get_character_info, Qt::CaseInsensitive)==0)
	{
		//ShowPopupMessage(QString::fromLocal8Bit("[웹->앱] get_character_info : ") + m_character_info); 
		elogd("[WebCommunicator::WebToAppCommand] return character_info(%s)", m_character_info.toStdString().c_str());
		return m_character_info;
	}
	else if (QString::compare(type, w2a_command::show_popup_message, Qt::CaseInsensitive)==0)
	{
		ShowPopupMessage(data);
	}

	return "";
}

void WebCommunicator::ShowPopupMessage(const QString& message)
{
	elogd("[WebCommunicator::ShowPopupMessage] message(%s)", message.toStdString().c_str());
	VideoChatMessageBox::Instance()->information(StringUtils::GetStringFromUtf8(message.toStdString()));
}

void WebCommunicator::ReportChannelCreation(int result, const QString& data)
{
	elogd("[WebCommunicator::ReportChannelCreation] result(%d) data(%s)", result, data.toStdString().c_str());
	WebviewControl::Instance()->ReportChannelCreation(result, data);
}	
	