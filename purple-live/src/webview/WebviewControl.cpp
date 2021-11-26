#include "WebviewControl.h"
#include "WebviewConfig.h"
#include "WebCommunicator.h"
#include "AppUtils.h"
#include "AppConfig.h"
#include "easylogging++.h"
#include "LocalSettings.h"
#include "HiddenSettings.h"
#include "WebviewConfig.h"
#include "VideoChatMessageBox.h"
#include "WebView.h"
#include "WebPage.h"
#include "browser.h"
#include "browserwindow.h"
#include "tabwidget.h"
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QVBoxLayout>
#include <QMenu>

WebviewControl::WebviewControl()
{

}

WebviewControl::~WebviewControl()
{
	elogd("[WebviewControl::~WebviewControl] Singleton");
}

void WebviewControl::ShowWebviewSample()
{
	//QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    //QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

	QWebEngineSettings::defaultSettings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    QWebEngineSettings::defaultSettings()->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
    QWebEngineProfile::defaultProfile()->setUseForGlobalCertificateVerification();
	
	BrowserWindow *window = m_browser.createWindow();
	QUrl url{LocalSettings::GetString(WebviewConfig::webview_sample_url_key, WebviewConfig::default_url)};
	window->tabWidget()->setUrl(url);
}

WebView* WebviewControl::CreateWebview(QWidget* parent)
{
	WebView* webview = new WebView(parent);
	auto profile = QWebEngineProfile::defaultProfile();
	auto webPage = new WebPage(profile, m_webview);
	profile->setRequestInterceptor(&interceptor);
	// User-Agent		ÆÛÇÃÁ¸¿ë User-Agent : PurpleLive/[version] (Windows)
	profile->setHttpUserAgent(QString("%1/%2 (Windows)").arg(AppConfig::APP_NAME, App()->GetAppVersion().c_str()));
	webview->setPage(webPage);
	return webview;
}

WebView* WebviewControl::GetPopupWebview()
{
	return m_webview;
}

void WebviewControl::ShowWebviewPopup(const QString& url)
{
	if (m_webview==nullptr)
	{
		m_webview = CreateWebview(&m_dialog);
		auto layout = new QHBoxLayout;
		layout->setMargin(0);
		layout->setSpacing(0);
		layout->addWidget(m_webview);
		m_dialog.setLayout(layout);
		m_dialog.setWindowFlags(Qt::WindowCloseButtonHint);
	}
   
	if (url.isEmpty())
	{
		m_webview->setUrl({LocalSettings::GetString(WebviewConfig::webview_sample_url_key, WebviewConfig::default_url)});
	}
	else
	{
		m_webview->setUrl(url);
	}
	
	m_dialog.show();
}

void WebviewControl::ShowWebviewDevTools(WebView* webview)
{
	CloseWebviewDevTools();
	m_browser_window = m_browser.createDevToolsWindow();
	webview->page()->setDevToolsPage(m_browser_window->currentTab()->page());
	webview->page()->triggerAction(QWebEnginePage::InspectElement);
}

void WebviewControl::CloseWebviewDevTools()
{
	if (m_browser_window!=nullptr)
	{
		m_browser_window->close();
		m_browser_window = nullptr;
	}
}

void WebviewControl::ShowContextMenu(WebView* webview, QPoint pos)
{
	if (!HiddenSettings::Instance()->GetEnableWebDebug())
		return;

	WebCommunicator::Instance()->SetWebView(webview);

	QMenu menu;
	QMenu* webview_menu = &menu;
	
	QAction* act_user_agent = webview_menu->addAction(QString::fromLocal8Bit("[¾Û->À¥] navigator.userAgent"));
	webview->connect(act_user_agent, &QAction::triggered, webview, []() {
		QString javascript("alert(navigator.userAgent);");
		WebCommunicator::Instance()->RunJavascript(javascript);
	});

	QAction* act_get_channel_id = webview_menu->addAction(QString::fromLocal8Bit("[¾Û->À¥] Ã¤³Î ID ¿äÃ»"));
	webview->connect(act_get_channel_id, &QAction::triggered, webview, []() {
		WebCommunicator::Instance()->AppToWebCommand("get_chatting_channel_id", "");
	});

	QAction* act_set_character_id = webview_menu->addAction(QString::fromLocal8Bit("[¾Û->À¥] LIVE.setCharacterId('id from app')"));
	webview->connect(act_set_character_id, &QAction::triggered, webview, [webview]() {
		QString id = MainWindow::Get()->GetStartupWindow()->GetWebviewCharacterId();
		WebviewControl::Instance()->SetCharacterId(webview, id);
	});

	QAction* act_get_character_id = webview_menu->addAction(QString::fromLocal8Bit("[¾Û->À¥] LIVE.getCharacterId()"));
	webview->connect(act_get_character_id, &QAction::triggered, webview, []() {
		QString javascript(QString("LIVE.getCharacterId();"));
		WebCommunicator::Instance()->RunJavascript(javascript, [javascript](const QVariant& result) {
			WebCommunicator::Instance()->ShowPopupMessage(QString("LIVE.getCharacterId() => %1").arg(result.toString()));
		});
	});

	QAction* act_set_character_info = webview_menu->addAction(QString::fromLocal8Bit("[¾Û->À¥] LIVE.setCharacterInfo('info from app')"));
	webview->connect(act_set_character_info, &QAction::triggered, webview, [webview]() {
		QString info = MainWindow::Get()->GetStartupWindow()->GetWebviewCharacterInfo();
		WebviewControl::Instance()->SetCharacterInfo(webview, info);
	});

	QAction* act_get_character_info = webview_menu->addAction(QString::fromLocal8Bit("[¾Û->À¥] LIVE.getCharacterInfo()"));
	webview->connect(act_get_character_info, &QAction::triggered, webview, []() {
		QString javascript(QString("LIVE.getCharacterInfo();"));
		WebCommunicator::Instance()->RunJavascript(javascript, [javascript](const QVariant& result) {
			WebCommunicator::Instance()->ShowPopupMessage(QString("LIVE.getCharacterInfo() => %1").arg(result.toString()));
		});
	});

	webview_menu->addSeparator();
	QAction* act_dev_tools = webview_menu->addAction(QString::fromLocal8Bit("°³¹ßÀÚ µµ±¸"));
	webview->connect(act_dev_tools, &QAction::triggered, [webview]() { 
		WebviewControl::Instance()->ShowWebviewDevTools(webview);
	});
	
	menu.exec(pos);
}

QString WebviewControl::MakeCharacterInfo(const QString& character_id, const QString& game_code)
{
/*
LIVE.setCharacterInfo(info) 
{
  gameUserId: string,
  gameCode: string,
  //token: string
}
*/
	std::string params = "";
	json_t* root = json_object();
	json_object_set(root, "gameUserId", json_string(character_id.toStdString().c_str()));
	json_object_set(root, "gameCode", json_string(game_code.toStdString().c_str()));
	params = json_dumps(root, 0);
	elogi("[WebviewControl::MakeCharacterInfo] info(%s)", params.c_str());
	return params.c_str();
}

void WebviewControl::SetCharacterInfo(WebView* webview, const QString& character_info)
{
	if (webview==nullptr)
		return;

	if (!WebCommunicator::Instance()->GetJavascriptLoaded())
	{
		elogi("[WebviewControl::SetCharacterInfo] skip GetJavascriptLoaded(0)");
		return;
	}

	QString javascript(QString("LIVE.setCharacterInfo(%1);").arg(character_info));
	elogi("[WebviewControl::SetCharacterInfo] info(%s) javascript(%s)", character_info.toStdString().c_str(), javascript.toStdString().c_str());
	//WebCommunicator::Instance()->ShowPopupMessage(javascript);
	WebCommunicator::Instance()->RunJavascript(javascript);
}

void WebviewControl::SetCharacterId(WebView* webview, const QString& character_id)
{
	if (webview==nullptr)
		return;

	if (!WebCommunicator::Instance()->GetJavascriptLoaded())
	{
		elogi("[WebviewControl::SetCharacterInfo] skip GetJavascriptLoaded(0)");
		return;
	}

	QString javascript(QString("LIVE.setCharacterId('%1');").arg(character_id));
	elogi("[WebviewControl::SetCharacterId] character_id(%s) javascript(%s)", character_id.toStdString().c_str(), javascript.toStdString().c_str());
	//WebCommunicator::Instance()->ShowPopupMessage(QString::fromLocal8Bit("[¾Û->À¥] Ä³¸¯ÅÍ ID¸¦ Àü´ÞÇÕ´Ï´Ù.") + QString("(%1)").arg(character_id));
	WebCommunicator::Instance()->RunJavascript(javascript);
}

void WebviewControl::SetShowWebviewWidget(bool show)
{
	MainWindow::Get()->GetStartupWindow()->SetShowWebviewWidget(show);
}

void WebviewControl::SetWebviewWidgetUrl(const QString& url, bool forced)
{
	MainWindow::Get()->GetStartupWindow()->SetWebviewUrl(url, forced);
}

void WebviewControl::ReportChannelCreation(int result, const QString& data)
{
	std::string result_message{};
	switch (result)
	{
		case web_result_code::success:
			result_message = "success";
			MainWindow::Get()->GetStartupWindow()->CreatedChattingChannel(data);
			SetShowWebviewWidget(false);
			break;
		case web_result_code::cancel:
			result_message = "cancel";
			SetShowWebviewWidget(false);
			break;
		case web_result_code::error:
		case web_result_code::error_network:
		case web_result_code::error_lime_response:
			result_message = "error";
			VideoChatMessageBox::Instance()->information(StringUtils::GetStringFromUtf8(data.toStdString()));
			break;
		default:
			result_message = "undefined";
	}

	elogd("[WebviewControl::ReportChannelCreation] result(%d, %s) data(%s)", result, result_message.c_str(), data.toStdString().c_str());
}