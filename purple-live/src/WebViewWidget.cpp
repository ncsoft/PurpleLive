#include "WebViewWidget.h"
#include "AppString.h"
#include "easylogging++.h"
#include "./webview/WebviewControl.h"

WebViewWidget::WebViewWidget(QWidget *parent)
	: StartUpPopUp(parent)
{
	ui.setupUi(this);

	SetChildWidget(ui.mainWidget);

	SetShowButton(false);
	
	m_webview = WebviewControl::Instance()->CreateWebview(ui.mainWidget);
	ui.webViewPaintLayout->addWidget(m_webview);
}

WebViewWidget::~WebViewWidget()
{
}

bool WebViewWidget::IsShow()
{
	return !GetHide();
}

void WebViewWidget::SetUrl(QString url, bool forced)
{
	elogd("[WebViewWidget::SetUrl] url(%s) forced(%d)", url.toStdString().c_str(), forced);
	if (forced || url != m_webview->url().toString())
		m_webview->setUrl(url);
}

WebView* WebViewWidget::GetWebview()
{
	return m_webview;
}

void WebViewWidget::on_popup_close_btn_clicked()
{
	StartUpPopUp::hide();
}

int WebViewWidget::GetStartUpPopUpY() 
{ 
	return (MainWindow::Get()->GetStartupWindow()->height()
		- MainWindow::Get()->GetStartupWindow()->getTitlebarHeight()
		- GetStartUpPopUpHeight()) / 2
		+ MainWindow::Get()->GetStartupWindow()->getTitlebarHeight();
}

int WebViewWidget::GetStartUpPopUpHeight() 
{ 
	return MainWindow::Get()->GetStartupWindow()->height() - MainWindow::Get()->GetStartupWindow()->getTitlebarHeight() - STARTPOPUP_Y_MARGIN * 2;
}

void WebViewWidget::UpdateLanguageStringCalled()
{
	SetLabel(AppString::GetQString("SlideUpDlg/make_new_chat"));
}