#pragma once

#include <Singleton.h>
#include "browser.h"
#include "WebviewDialog.h"
#include "WebviewUrlRequestInterceptor.h"

class WebView;
class WebviewControl : public CommunityLiveCore::Singleton<WebviewControl>
{
public:
	explicit WebviewControl();
	virtual ~WebviewControl();	

	WebView* CreateWebview(QWidget* parent);
	WebView* GetPopupWebview();
	void ShowWebviewSample();
	void ShowWebviewPopup(const QString& url = "");
	void ShowWebviewDevTools(WebView* webview);
	void CloseWebviewDevTools();
	void ShowContextMenu(WebView* webview, QPoint pos);
	
	QString MakeCharacterInfo(const QString& character_id, const QString& game_code);
	void SetCharacterInfo(WebView* webview, const QString& character_info);
	void SetCharacterId(WebView* webview, const QString& character_id);
	void SetShowWebviewWidget(bool show);
	void SetWebviewWidgetUrl(const QString& url, bool forced);
	void ReportChannelCreation(int result, const QString& data);
	 
private:
	Browser m_browser;
	WebviewDialog m_dialog;
	WebviewUrlRequestInterceptor interceptor;
	WebView* m_webview = nullptr;
	BrowserWindow* m_browser_window = nullptr;
};