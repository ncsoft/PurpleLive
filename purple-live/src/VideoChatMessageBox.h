#pragma once

#include <qwidget.h>
#include <qtimer.h>
#include <qlabel.h>
#include "AppConfig.h"
#include "ui_VideoChatMessageBox.h"

#include "ScreenLockWidget.h"
#include "DimWidget.h"
#include "QFramelessPopup.h"

#define MSG_BOX_FONT_SIZE				13
#define MSG_BOX_TOP_MARGIN				42
#define MSG_BOX_BOTTOM_MARGIN			78
#define MSG_BOX_CLOSE_BTN_ICON_SIZE		20
#define MSG_BOX_MIN_WIDTH				280
#define MSG_BOX_MIN_HEIGHT				180
#define MSG_BOX_TEXT_MAIN_HEIGHT		20
#define MSG_BOX_TEXT_SUB_HEIGHT			16
#define WIDTH_PADDING					80		
#define SHADOW_X_MARGIN					10
#define SHADOW_Y_MARGIN					24
#define SHADOW_COLOR					QColor(0, 0, 0, 255 * 0.08)

/*
// use like below
#include "VideoChatMessageBox.h"
// ...
VideoChatMessageBox::Instance()->information("Message");
// ...
*/

struct stimerData {
	QString text;
	int time;
};

class VideoChatMessageBox : public QFramelessPopup
{
	Q_OBJECT

public:
	VideoChatMessageBox(QWidget *parent = Q_NULLPTR);
	virtual ~VideoChatMessageBox();

	void information(const std::string &mainText, const std::string &subText = {});
	bool question(const std::string &text);
	bool selection(const std::string &text, const std::string &button1, const std::string &button2);
	void information_stimer(const std::string &mainText, const std::string &subText = {}, int _time = -1);
	void SetDimWidget(bool enable);
	void ChangeInformText();

	static VideoChatMessageBox* Instance()
	{
		VideoChatMessageBox* instance = new VideoChatMessageBox;
		connect(ScreenLockWidget::Instance(), &ScreenLockWidget::SendScreenLockShowEvent, instance, &VideoChatMessageBox::OnScreenLockShowEvent);
		connect(ScreenLockWidget::Instance(), &ScreenLockWidget::SendScreenLockHidedEvent, instance, &VideoChatMessageBox::OnScreenLockHideEvent);
		return instance;
	}

public slots:
	void ResetGeometry();
	void OnScreenLockShowEvent();
	void OnScreenLockHideEvent();
	void exec();

private:
	Ui::VideoChatMessageBox ui;

	int									m_currentWidth = MSG_BOX_MIN_WIDTH;
	int									m_currentHeight = MSG_BOX_MIN_HEIGHT;
	bool								m_parentHide = false;
	bool								m_answer = false;
	QWidget*							m_parent	= Q_NULLPTR;
	std::unique_ptr<dimmingWidget>		m_dimWidget;
	std::unique_ptr<dimmingPainter>		m_dimPainter;
	QTimer								m_timer;
	stimerData							m_timerData;
	vector<QLabel*>						m_vecInformMessage;
	vector<QLabel*>						m_vecInformSubMessage;
	QString								m_InformMainMessage;
	QString								m_InformSubMessage;
	QPointer<QEventLoop>				m_loop;

	void InitUI();
	void SetGeometry(const QRect& rect);
	int SetInformLabelMessage(QString message, bool isMainMessage = true);

	virtual const int getTitlebarHeight() { return 0; }
	virtual const int getTitlebarWidth() { return 0; }

private slots:
	void on_close_btn_clicked();
	void on_ok_btn_clicked();
	void on_cancel_btn_clicked();
	void OnTimerCallbackFunc();
};
