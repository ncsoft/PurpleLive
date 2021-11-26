#include "QFramelessDialog.h"

#include <Windows.h>					// for window MSG
#include <qmouseeventtransition.h>		// for qt-mouse-event
#include <dwmapi.h>						// for dwm-api
#include <qtimer.h>

#pragma comment (lib,"Dwmapi.lib")

QFramelessDialog::QFramelessDialog(QWidget *parent, bool _isFrameless, uint8_t _borderwidth, bool _useResize, float _aspectratio)
	: QMainWindow(parent), m_borderWidth(_borderwidth), m_enableResize(_useResize), m_aspectRatio(_aspectratio)
{

	if (_isFrameless) {
		// 최대화를 지원한다면
		//setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);
		setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

		setAttribute(Qt::WA_OpaquePaintEvent, false);
		setAttribute(Qt::WA_PaintOnScreen, true);
		setAttribute(Qt::WA_DontCreateNativeAncestors, true);
		setAttribute(Qt::WA_NativeWindow, true);
		setAttribute(Qt::WA_NoSystemBackground, true);
		setAttribute(Qt::WA_MSWindowsUseDirect3D, true);
		setAutoFillBackground(true);

		m_doubleClickInterval = GetDoubleClickTime();
	}
	else {
	}
}

bool QFramelessDialog::nativeEvent(const QByteArray &eventType, void *message, long *result) 
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
		if (params.rgrc[0].top != 0 && m_enableResize) {
			params.rgrc[0].bottom += 1;
		}
		
		//this kills the window frame and title bar we added with WS_THICKFRAME and WS_CAPTION
		*result = WVR_REDRAW;
		return true;
	}
	case WM_SIZING:
	{
		if (m_aspectRatio == 0) break;
		RECT* rect = (RECT*)msg->lParam;
		LONG width = rect->right - rect->left;
		LONG height = rect->bottom - rect->top;

		/*
		// if you want to setting min or max width, height
		// try to set below
		width = std::max(width, minWidth);
		width = std::min(width, maxWidth);
		height = std::max(height, minHeight);
		height = std::min(height, maxHeight);
		*/

		switch (msg->wParam)
		{
		case WMSZ_LEFT:
		case WMSZ_BOTTOMLEFT:
			rect->left = rect->right - width;
			rect->bottom = rect->top + width / m_aspectRatio;
			break;
		case WMSZ_RIGHT:
		case WMSZ_BOTTOMRIGHT:
			rect->right = rect->left + width;
			rect->bottom = rect->top + width / m_aspectRatio;
			break;
		case WMSZ_TOP:
		case WMSZ_TOPRIGHT:
			rect->top = rect->bottom - height;
			rect->right = rect->left + height * m_aspectRatio;
			break;
		case WMSZ_BOTTOM:
			rect->bottom = rect->top + height;
			rect->right = rect->left + height * m_aspectRatio;
			break;
		case WMSZ_TOPLEFT:
			rect->left = rect->right - width;
			rect->top = rect->bottom - width / m_aspectRatio;
			break;
		}
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
	return QMainWindow::nativeEvent(eventType, message, result);
}

bool QFramelessDialog::event(QEvent *event)
{
	QPoint widgetCursorPos = QWidget::mapFromGlobal(QCursor::pos());

	int x = widgetCursorPos.x();
	int y = widgetCursorPos.y();
	QSize size = this->size();

	switch (event->type())
	{
	case QEvent::HoverEnter:
	case QEvent::HoverMove:
		if (!m_enableResize) break;
		if (x < m_borderWidth && y > size.height() - m_borderWidth) {
			this->setCursor(QCursor(Qt::SizeBDiagCursor));
		}
		else if (x > size.width() - m_borderWidth && y > size.height() - m_borderWidth) {
			this->setCursor(QCursor(Qt::SizeFDiagCursor));
		}
		else if (x < m_borderWidth && y < m_borderWidth) {
			this->setCursor(QCursor(Qt::SizeFDiagCursor));
		}
		else if (x > size.width() - m_borderWidth && y < m_borderWidth) {
			this->setCursor(QCursor(Qt::SizeBDiagCursor));
		}
		else if (x < m_borderWidth) {
			this->setCursor(QCursor(Qt::SizeHorCursor));
		}
		else if (y < m_borderWidth) {
			this->setCursor(QCursor(Qt::SizeVerCursor));
		}
		else if (y > size.height() - m_borderWidth) {
			this->setCursor(QCursor(Qt::SizeVerCursor));
		}
		else if (x > size.width() - m_borderWidth) {
			this->setCursor(QCursor(Qt::SizeHorCursor));
		}
		else {
			this->setCursor(QCursor(Qt::ArrowCursor));
		}
		return true;
		break;
	case QEvent::HoverLeave:
		this->setCursor(QCursor(Qt::ArrowCursor));
		return true;
		break;
	case QEvent::MouseButtonPress:
		if (!m_enableResize) {
			if (y < getTitlebarHeight()) {
				ReleaseCapture();
				SendMessage(HWND(winId()), WM_NCLBUTTONDOWN, HTCAPTION, 0);
			}
			break;
		}
		ReleaseCapture();
		if (x < m_borderWidth && y > size.height() - m_borderWidth) {
			this->setCursor(QCursor(Qt::SizeBDiagCursor));
			SendMessage(HWND(winId()), WM_NCLBUTTONDOWN, HTBOTTOMLEFT, 0);
		}
		else if (x > size.width() - m_borderWidth && y > size.height() - m_borderWidth) {
			this->setCursor(QCursor(Qt::SizeFDiagCursor));
			SendMessage(HWND(winId()), WM_NCLBUTTONDOWN, HTBOTTOMRIGHT, 0);
		}
		else if (x < m_borderWidth && y < m_borderWidth) {
			this->setCursor(QCursor(Qt::SizeFDiagCursor));
			SendMessage(HWND(winId()), WM_NCLBUTTONDOWN, HTTOPLEFT, 0);
		}
		else if (x > size.width() - m_borderWidth && y < m_borderWidth) {
			this->setCursor(QCursor(Qt::SizeBDiagCursor));
			SendMessage(HWND(winId()), WM_NCLBUTTONDOWN, HTTOPRIGHT, 0);
		}
		else if (y < m_borderWidth) {
			this->setCursor(QCursor(Qt::SizeVerCursor));
			SendMessage(HWND(winId()), WM_NCLBUTTONDOWN, HTTOP, 0);
		}
		else if (x < m_borderWidth) {
			this->setCursor(QCursor(Qt::SizeHorCursor));
			SendMessage(HWND(winId()), WM_NCLBUTTONDOWN, HTLEFT, 0);
		}
		else if (y > size.height() - m_borderWidth) {
			this->setCursor(QCursor(Qt::SizeVerCursor));
			SendMessage(HWND(winId()), WM_NCLBUTTONDOWN, HTBOTTOM, 0);
		}
		else if (x > size.width() - m_borderWidth) {
			this->setCursor(QCursor(Qt::SizeHorCursor));
			SendMessage(HWND(winId()), WM_NCLBUTTONDOWN, HTRIGHT, 0);
		}
		else if (y < getTitlebarHeight()) {
			if (m_isClicked && m_enableMaximize)
			{
				m_isClicked = false;
				if (isMaximized())
				{
					setWindowState(Qt::WindowNoState);
				}
				else
				{
					setWindowState(Qt::WindowMaximized);
				}
			}
			else
			{
				m_isClicked = true;
				SendMessage(HWND(winId()), WM_NCLBUTTONDOWN, HTCAPTION, 0);
			}

			QTimer::singleShot(m_doubleClickInterval * 0.7 , this, [=]() {
				m_isClicked = false;
			});
		}
		break;
	case QEvent::Move:
		m_isClicked = false;
		break;
	default:
		break;
	}
	return QWidget::event(event);
}

void QFramelessDialog::SetEraseBackGround(QColor color)
{
	m_eraseBackGround = true;
	m_backGroundColor = color;
}