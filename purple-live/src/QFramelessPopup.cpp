#include "QFramelessPopup.h"

#include <Windows.h>					// for window MSG
#include <qmouseeventtransition.h>		// for qt-mouse-event
#include <dwmapi.h>						// for dwm-api

#pragma comment (lib,"Dwmapi.lib")

QFramelessPopup::QFramelessPopup(QWidget *parent, bool _isFrameless)
	: QWidget(parent)
{

	if (_isFrameless) {
		// 최대화를 지원한다면
		//setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);
		setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

		setAttribute(Qt::WA_OpaquePaintEvent, false);
		//setAttribute(Qt::WA_PaintOnScreen, true);
		setAttribute(Qt::WA_DontCreateNativeAncestors, true);
		setAttribute(Qt::WA_NativeWindow, true);
		setAttribute(Qt::WA_NoSystemBackground, true);
		setAttribute(Qt::WA_MSWindowsUseDirect3D, true);
		setAutoFillBackground(true);
	}
	else {
	}
}

bool QFramelessPopup::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
#if (QT_VERSION == QT_VERSION_CHECK(5, 11, 1))
	MSG* msg = *reinterpret_cast<MSG**>(message);
#else
	MSG* msg = reinterpret_cast<MSG*>(message);
#endif

	switch (msg->message)
	{
	case WM_NCPAINT:
	{
		// 필요할 경우 사용
		return false;
	}
	case WM_NCCALCSIZE:
	{
		NCCALCSIZE_PARAMS& params = *reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam);
		if (params.rgrc[0].top != 0) {
			params.rgrc[0].bottom += 1;
		}
		
		//this kills the window frame and title bar we added with WS_THICKFRAME and WS_CAPTION
		*result = WVR_REDRAW;
		return true;
	}
	case WM_ERASEBKGND:
	{
		// BackGround를 같은 색상으로 덮는다
		if (!m_eraseBackGround) break;
		RECT r;
		GetClientRect(HWND(winId()), &r);
		HDC h_dc = GetDC(HWND(winId()));
		HBRUSH bru;
		bru = CreateSolidBrush(RGB(m_backGroundColor.red(), m_backGroundColor.green(), m_backGroundColor.blue()));
		FillRect(h_dc, &r, bru);
		DeleteObject(bru);
		ReleaseDC(HWND(winId()), h_dc);
	}
	default:
		break;
	}
	return QWidget::nativeEvent(eventType, message, result);
}

bool QFramelessPopup::event(QEvent *event)
{
	QPoint widgetCursorPos = QWidget::mapFromGlobal(QCursor::pos());

	int x = widgetCursorPos.x();
	int y = widgetCursorPos.y();
	QSize size = this->size();

	switch (event->type())
	{
	case QEvent::MouseButtonPress:
		if (y < getTitlebarHeight()) {
			ReleaseCapture();
			SendMessage(HWND(winId()), WM_NCLBUTTONDOWN, HTCAPTION, 0);
		}
		break;
		ReleaseCapture();
		return true;
	default:
		break;
	}
	return QWidget::event(event);
}

void QFramelessPopup::SetEraseBackGround(QColor color)
{
	m_eraseBackGround = true;
	m_backGroundColor = color;
}