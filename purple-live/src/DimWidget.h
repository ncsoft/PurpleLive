#pragma once

#include <qcombobox.h>
#include "lime-defines.h"

#include <qdialog.h>
#include <qtimer.h>

#define		CIRCLE_INDICATOR_ICON_SIZE				24
#define		CIRCLE_INDICATOR_REPAINT_INTERVAL		50ms
#define		CIRCLE_INDICATOR_CHANGE_ANGLE			5

class dimmingPainter : public QWidget {
	Q_OBJECT

public:
	dimmingPainter(QWidget *parent = 0);
	virtual ~dimmingPainter();

	void SetPaintRect(QRect& r) { m_paintRect = r; setGeometry(m_paintRect); }
	void SetPaintColor(QColor& c) { m_paintColor = c; }
	void SetChild(QWidget* messageBox);
	void ShowIndicator();
	void HideIndicator();
	void Hide();

private:
	QWidget* m_parent{};
	QWidget* m_messageBox{};
	QRect m_paintRect{};
	QColor m_paintColor{};
	QPixmap m_circleIndicatorImage{};
	QTimer m_indicatorDrawTimer{};
	int m_indicatorAngle = 0;

private slots:
	void IndicatorDrawTimeout();
	void paintEvent(QPaintEvent* e);
	bool eventFilter(QObject* obj, QEvent* event);
};

class dimmingWidget : public QDialog {
	Q_OBJECT
public:
	explicit dimmingWidget(QWidget *parent = 0);
	virtual ~dimmingWidget() { }
	void setCurrentGeo(QWidget* _w, int margin_x = 0, int margin_y = 0, int margin_w = 0, int margin_h = 0);
	void DimWidgetCntUp() { mDimWidgetCnt++; }
	void DimWidgetCntDown() { if (mDimWidgetCnt > 0) mDimWidgetCnt--; }
	size_t getDimWidgetCnt() { return mDimWidgetCnt; }
	void SetEnableResize(bool b) { m_enableResize = b; }
	void SetEnableMoveWidget(bool b) { m_enableMoveWidget = b; }
	void SetBorderWidth(int border) { m_borderWidth = border; }
	void SetChild(QWidget* widget) { m_child = widget; }
	void SetTitleHeight(int height) { m_titleHeight = height; }

public slots:
	void ResetGeometry();
	void RaiseChildWidget();

private slots:
	bool nativeEvent(const QByteArray &eventType, void *message, long *result);
	bool event(QEvent *event);

protected:
	size_t mDimWidgetCnt = 0;

private:
	bool m_enableResize = false;
	bool m_enableMoveWidget = false;
	int m_borderWidth = 5;		// default border width
	int m_titleHeight = 40;		// default titlebar height

	bool m_titleBarClicked = false;

	bool m_topResize = false;
	bool m_bottomResize = false;
	bool m_leftResize = false;
	bool m_rightResize = false;
	bool m_topLeftResize = false;
	bool m_topRightResize = false;
	bool m_bottomLeftResize = false;
	bool m_bottomRightResize = false;
	
	int m_pastX = 0;
	int m_pastY = 0;
	int m_pastMouseX = 0;
	int m_pastMouseY = 0;
	int m_pastGlobalMouseX = 0;
	int m_pastGlobalMouseY = 0;
	int m_pastWidth = 0;
	int m_pastHeight = 0;

	int m_marginX = 0;
	int m_marginY = 0;
	int m_marginWidth = 0;
	int m_marginHeight = 0;
	
	QWidget* m_parent = nullptr;
	QWidget* m_child = nullptr;

	void ReleaseMouse();
	bool IsResizing();

signals:
	void ResetChildGeometry();
};