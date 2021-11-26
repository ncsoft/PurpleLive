#pragma once

#include "StartUpPopUp.h"
#include "ui_WebViewWidget.h"
#include "./webview/webview.h"

class WebViewWidget : public StartUpPopUp
{
	Q_OBJECT

public:
	WebViewWidget(QWidget *parent = Q_NULLPTR);
	virtual ~WebViewWidget();
	
	bool IsShow();
	void SetUrl(QString url, bool forced);
	WebView* GetWebview();

private:
	Ui::WebViewWidget ui;
	WebView* m_webview = nullptr;

private slots:
	void on_popup_close_btn_clicked();
	void on_cancel_btn_clicked() {};	// button event
	void on_select_btn_clicked() {};	// button event

protected:
	virtual void ShowEventCalled() {}
	virtual void HideEventCalled() {}
	virtual int GetStartUpPopUpY() override;
	virtual int GetStartUpPopUpHeight() override;
	virtual void UpdateLanguageStringCalled() override;
};