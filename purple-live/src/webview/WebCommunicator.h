#pragma once
#include "Singleton.h"
#include "webview.h"
#include "AppUtils.h"
#include "easylogging++.h"

#include <QWebEngineView>
#include <QWebChannel>


#define APP_URI_SCHEME "purpleliveapp"

enum web_result_code {
    success= 0,             // success
    cancel= 1,              // user cancel
    error= 2,               // creation fail
    error_network= 3,       // network error
    error_lime_response= 4, // LIME response error
};

struct w2a_command
{
	static constexpr const char* log = "log";
	static constexpr const char* get_character_id = "get_character_id";
	static constexpr const char* get_character_info = "get_character_info";
	static constexpr const char* show_popup_message = "show_popup_message";
};

struct a2w_command
{
	static constexpr const char* get_chatting_channel_id = "get_chatting_channel_id";
};

class WebCommunicator : public CommunityLiveCore::Singleton<WebCommunicator>
{
public:
	explicit WebCommunicator();
	virtual ~WebCommunicator();	

	void SetWebView(WebView* webview);
	void SetWebChannel(WebView* webview);
	void SetCharactorId(const QString& charactor_id);
	void SetCharactorInfo(const QString& charactor_info);
	
	void InitUriScheme(QWebEngineProfile* profile);
	bool FilterUriScheme(const QUrl& url);

	bool GetJavascriptLoaded();
	void SetJavascriptLoaded(bool loaded);
	void RunJavascript(const QString& javascript, const QWebEngineCallback<const QVariant &> &result_callback = QWebEngineCallback<const QVariant&>());
	void AppToWebCommand(const QString& type, const QString& data, const QWebEngineCallback<const QVariant&> &resultCallback = QWebEngineCallback<const QVariant&>());
	QString WebToAppCommand(const QString& type, const QString& data);
	void ReportChannelCreation(int result, const QString& data);
	void ShowPopupMessage(const QString& message);
	
private:
	bool m_javascript_loaded = false;
	WebView* m_webview = nullptr;
	QString m_character_id = "[character-id from app]";
	QString m_character_info = "[character-info from app]";
};
