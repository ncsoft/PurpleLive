#pragma once

#include <qtimer.h>

#include "ui_StartUpPopUp.h"

#define STARTPOPUP_X_MARGIN						24
#define STARTPOPUP_Y_MARGIN						24

#define STARTPOPUP_CHILD_TOP_MARGIN				74
#define STARTPOPUP_CHILD_BOTTOM_MARGIN			92
#define STARTPOPUP_CHILD_TOP_BOTTOM_MARGIN		( STARTPOPUP_CHILD_TOP_MARGIN + STARTPOPUP_CHILD_BOTTOM_MARGIN )

#define FADEOUT_TIMER_INTERVAL					3
#define FADEOUT_DRAW_CNT						7.0

class StartUpPopUp : public QWidget
{
	Q_OBJECT

public:
	StartUpPopUp(QWidget *parent = Q_NULLPTR);
	virtual ~StartUpPopUp();
	void SetLabel(QString str) { ui.slideupTitle_label->setText(str); }
	void SetShowButton(bool show);
	void SetChildWidget(QWidget* w);
	bool GetHide() { return m_bHide; }

public slots:
	void show();
	void hide();
	void ResetGeometry();
	void UpdateLanguageString();
	void OnScreenLockShowEvent();
	void OnScreenLockHideEvent();

private:
	Ui::StartUpPopUp ui;

	QWidget*		m_child = Q_NULLPTR;
	QTimer			m_timer{};
	bool			m_bHide = true;
	bool			m_pastIsVisible = false;
	bool			m_showButton = true;
	unsigned int	m_drawCnt = 0;
	QPixmap			m_screenCapture{};

	void InitUI();
	int GetStartUpPopUpWidth();
	QRect GetStartUpPopUpRect();
	void ShowDimWidget(bool bshow);
	void ActiveStartUpPopUp(bool bshow);
	void SetGeoMetry(QRect& r);
	void HideAllItems(bool hide);
	void CaptureCurrentScreen();
	void StopFading(bool show);
	void TransparentMainWidget(bool transparent);
	void DrawImage(QPixmap* pixmap, QPainter* painter, int _x, int _y, int _width, int _height);

private slots:
	void Timeout();
	void showEvent(QShowEvent* e);

protected:
	void SetHideButtonWidget() { ui.buttonWidget->hide(); }
	void SetTextCancelBtn(QString text);
	void SetTextSelectBtn(QString text);
	virtual void ShowEventCalled() abstract;
	virtual void HideEventCalled() abstract;
	virtual int GetStartUpPopUpY() abstract;
	virtual int GetStartUpPopUpHeight() abstract;
	virtual void UpdateLanguageStringCalled() abstract;

protected slots:
	bool eventFilter(QObject* obj, QEvent* event);
};