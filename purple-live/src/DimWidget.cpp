#include "DimWidget.h"
#include "MainWindow.h"
#include "StartUpWindow.h"
#include "VideoChatMessageBox.h"
#include "AppUtils.h"

#include <Windows.h>

#include <qevent.h>
#include <qpainter.h>

dimmingPainter::dimmingPainter(QWidget *parent) :
	QWidget(parent),
	m_paintRect(parent->x(), parent->y(), parent->width(), parent->height()),
	m_paintColor(QColor(0,0,0,255*0.5)),
	m_parent(parent)
{
	parent->installEventFilter(this);
	m_circleIndicatorImage = QPixmap(":/StartStreamDlg/image/ic_loading.svg");
}

dimmingPainter::~dimmingPainter()
{
	
}

void dimmingPainter::paintEvent(QPaintEvent* e)
{
	QPainter painter(this);

	painter.fillRect(QRect(0, 0, m_paintRect.width(), m_paintRect.height()), m_paintColor);

	if (m_circleIndicatorImage.isNull() == false &&
		m_indicatorDrawTimer.isActive())
	{
		DrawUtils::DrawRotatedImage(&m_circleIndicatorImage, &painter,
			QRect(
				width() / 2 - CIRCLE_INDICATOR_ICON_SIZE / 2,
				height() / 2 - CIRCLE_INDICATOR_ICON_SIZE / 2,
				CIRCLE_INDICATOR_ICON_SIZE, CIRCLE_INDICATOR_ICON_SIZE), m_indicatorAngle);
	}

	painter.end();
}

void dimmingPainter::SetChild(QWidget* messageBox)
{ 
	m_messageBox = messageBox;
	if(messageBox) HideIndicator();
}

void dimmingPainter::ShowIndicator()
{
	if (m_indicatorDrawTimer.isActive()) return;
	m_indicatorAngle = 0;
	connect(&m_indicatorDrawTimer, &QTimer::timeout, this, &dimmingPainter::IndicatorDrawTimeout);
	m_indicatorDrawTimer.setInterval(CIRCLE_INDICATOR_REPAINT_INTERVAL);
	m_indicatorDrawTimer.start();
}

void dimmingPainter::HideIndicator()
{
	m_indicatorDrawTimer.stop();
	update();
}

void dimmingPainter::IndicatorDrawTimeout()
{
	m_indicatorAngle += CIRCLE_INDICATOR_CHANGE_ANGLE;
	update();
}

void dimmingPainter::Hide()
{
	HideIndicator();
	QWidget::hide();
}

bool dimmingPainter::eventFilter(QObject* obj, QEvent* event)
{
	if (obj == m_parent)
	{
		if (	event->type() == QEvent::MouseButtonDblClick ||
				event->type() == QEvent::MouseButtonPress ||
				event->type() == QEvent::WindowActivate ||
				event->type() == QEvent::Paint ||
				event->type() == QEvent::NonClientAreaMouseButtonPress)
		{
			if (m_messageBox)
			{
				m_messageBox->raise();
			}
		}
		else if (event->type() == QEvent::Resize)
		{
			QResizeEvent *resizeEvent = static_cast<QResizeEvent*>(event);
			QRect r = { x(),
				y() + MainWindow::Get()->GetStartupWindow()->getTitlebarHeight(),
				width(), resizeEvent->size().height() - MainWindow::Get()->GetStartupWindow()->getTitlebarHeight() };
			SetPaintRect(r);
		}
		else if (event->type() == QEvent::Close)
		{
			if (m_messageBox)
			{
				m_messageBox->hide();
				QApplication::quit();
			}
		}
		else if (event->type() == QEvent::WindowStateChange)
		{
			if (m_messageBox)
			{
				switch (m_parent->windowState())
				{
				case Qt::WindowMinimized:
					m_messageBox->setWindowState(Qt::WindowMinimized);
					break;
				case Qt::WindowNoState:
					m_messageBox->setWindowState(Qt::WindowNoState);
					break;
				}
			}
		}
	}

	return QWidget::eventFilter(obj, event);
}

dimmingWidget::dimmingWidget(QWidget *parent) : QDialog(parent)
{
	setWindowFlags(windowFlags() | Qt::Tool | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint);
	setWindowOpacity(0.5);
	setObjectName("DimWidget");
	setStyleSheet("QWidget#DimWidget { background: black; }");

	connect(MainWindow::Get()->GetStartupWindow(), &StartUpWindow::ResetGeometry, this, &dimmingWidget::ResetGeometry);
}

void dimmingWidget::ResetGeometry()
{
	if (isHidden()) return;
	if (m_parent) setCurrentGeo(m_parent, m_marginX, m_marginY, m_marginWidth, m_marginHeight);
}

void dimmingWidget::setCurrentGeo(QWidget* _w, int margin_x, int margin_y, int margin_w, int margin_h)
{
	m_marginX = margin_x;
	m_marginY = margin_y;
	m_marginWidth = margin_w;
	m_marginHeight = margin_h;
	m_parent = _w;

	int _x = _w->geometry().x();
	int _y = _w->geometry().y();
	int _width = _w->geometry().width();
	int _height = _w->geometry().height();
	setGeometry(_x + m_marginX, _y + m_marginY, _width + m_marginWidth, _height + m_marginHeight);

}

bool dimmingWidget::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
	if (eventType != "windows_generic_MSG") {
		return QDialog::nativeEvent(eventType, message, result);
	}

	const MSG *msg = reinterpret_cast<MSG *>(message);

	switch (msg->message)
	{
	case WM_MOVE:
	case WM_SIZING:
	case WM_SIZE:
		if (m_child) emit ResetChildGeometry();
		break;
	case WM_NCCALCSIZE:
	{
		//this kills the window frame and title bar we added with
		//WS_THICKFRAME and WS_CAPTION
		*result = 0;
		return true;
	}
	case WM_MOUSEMOVE:
	{
		QPoint widgetCursorPos = QWidget::mapFromGlobal(QCursor::pos());

		int x = widgetCursorPos.x();
		int y = widgetCursorPos.y();
		QSize size = this->size();

		if (!m_enableResize || IsResizing()) break;
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
	}
	case WM_ERASEBKGND:
	{
		RECT r;
		GetClientRect(HWND(winId()), &r);
		HDC h_dc = GetDC(HWND(winId()));
		HBRUSH bru;
		bru = CreateSolidBrush(RGB(0, 0, 0));
		FillRect(h_dc, &r, bru);
		DeleteObject(bru);
		ReleaseDC(HWND(winId()), h_dc);
	}
	default: {
		break;
	}
	}
	return QDialog::nativeEvent(eventType, message, result);
}

void dimmingWidget::ReleaseMouse()
{
	m_titleBarClicked = false;
	m_topResize = false;
	m_bottomResize = false;
	m_leftResize = false;
	m_rightResize = false;
	m_topLeftResize = false;
	m_topRightResize = false;
	m_bottomLeftResize = false;
	m_bottomRightResize = false;
}

bool dimmingWidget::IsResizing()
{
	return 	m_topResize || m_bottomResize || m_leftResize || m_rightResize || m_topLeftResize || m_topRightResize || m_bottomLeftResize || m_bottomRightResize;
}

bool dimmingWidget::event(QEvent *event)
{
	QPoint widgetCursorPos = QWidget::mapFromGlobal(QCursor::pos());

	int x = widgetCursorPos.x();
	int y = widgetCursorPos.y();
	QSize size = this->size();

	switch (event->type())
	{
	case QEvent::MouseButtonRelease:
		ReleaseMouse();
		break;
	case QEvent::MouseMove:
	{
		QRect r;
		bool tresize = false;

		int min_height = static_cast<QWidget*>(parent())->minimumHeight();
		int min_width = static_cast<QWidget*>(parent())->minimumWidth();

		int margin_x = QCursor::pos().x() - m_pastGlobalMouseX;
		int margin_y = QCursor::pos().y() - m_pastGlobalMouseY;

		if (m_enableMoveWidget) {
			if (m_titleBarClicked) {
				r = QRect(m_pastX + margin_x, m_pastY + margin_y, m_pastWidth, m_pastHeight);
				tresize = true;
			}

			if (tresize) {
				if (r.width() < min_width) r.setWidth(min_width);
				if (r.height() < min_height) r.setHeight(min_height);
				setGeometry(r);
				static_cast<QWidget*>(parent())->setGeometry(r);
			}
			break;
		}

		if (m_enableResize) {
			if (m_titleBarClicked) {
				r = QRect(m_pastX + margin_x, m_pastY + margin_y, m_pastWidth, m_pastHeight);
				tresize = true;
			}
			else if (m_topResize) {				// top
				if (m_pastGlobalMouseY < QCursor::pos().y()) {
					if (min_height <= m_pastHeight - margin_y) {
						r = QRect(m_pastX, QCursor::pos().y(), m_pastWidth, m_pastHeight + (m_pastGlobalMouseY - QCursor::pos().y()));
						tresize = true;
					}
				}
				else if (m_pastGlobalMouseY > QCursor::pos().y()) {
					r = QRect(m_pastX, QCursor::pos().y(), m_pastWidth, m_pastHeight + (m_pastGlobalMouseY - QCursor::pos().y()));
					tresize = true;
				}
			}
			else if (m_bottomResize) {		// bottom
				r = QRect(m_pastX, m_pastY, m_pastWidth, m_pastHeight - (m_pastMouseY - y));
				tresize = true;
			}
			else if (m_leftResize) {		// left
				if (m_pastGlobalMouseX < QCursor::pos().x()) {
					if (min_width <= m_pastWidth - margin_x) {
						r = QRect(QCursor::pos().x(), m_pastY, m_pastWidth + (m_pastGlobalMouseX - QCursor::pos().x()), m_pastHeight);
						tresize = true;
					}
				}
				else if (m_pastGlobalMouseX > QCursor::pos().x()) {
					r = QRect(QCursor::pos().x(), m_pastY, m_pastWidth + (m_pastGlobalMouseX - QCursor::pos().x()), m_pastHeight);
					tresize = true;
				}
			}
			else if (m_rightResize) {		// right
				r = QRect(m_pastX, m_pastY, m_pastWidth - (m_pastMouseX - x), m_pastHeight);
				tresize = true;
			}
			else if (m_topLeftResize) {		// top-left 
				int setX = m_pastX;
				int setY = m_pastY;

				// top resize
				if (m_pastGlobalMouseY < QCursor::pos().y()) {

					if (min_height <= m_pastHeight - margin_y) {
						setY = QCursor::pos().y();
						tresize = true;
					}
					else {
						setY = m_pastY + m_pastHeight - min_height;
					}
				}
				else if (m_pastGlobalMouseY > QCursor::pos().y()) {
					setY = QCursor::pos().y();
					tresize = true;
				}

				// left resize
				if (m_pastGlobalMouseX < QCursor::pos().x()) {
					if (min_width <= m_pastWidth - margin_x) {
						setX = QCursor::pos().x();
						tresize = true;
					}
					else {
						setX = m_pastX + m_pastWidth - min_width;
					}
				}
				else if (m_pastGlobalMouseX > QCursor::pos().x()) {
					setX = QCursor::pos().x();
					tresize = true;
				}

				r = QRect(setX, setY, m_pastWidth + (m_pastGlobalMouseX - setX), m_pastHeight + (m_pastGlobalMouseY - setY));
			}
			else if (m_topRightResize)		// top-right
			{
				int setY = m_pastY;

				// top resize
				if (m_pastGlobalMouseY < QCursor::pos().y()) {
					if (min_height <= m_pastHeight - margin_y) {
						setY = QCursor::pos().y();
					}
					else {
						setY = m_pastY + m_pastHeight - min_height;
					}
				}
				else if (m_pastGlobalMouseY > QCursor::pos().y()) {
					setY = QCursor::pos().y();
				}

				r = QRect(m_pastX, setY, m_pastWidth - (m_pastMouseX - x) /* right resize */, m_pastHeight + (m_pastGlobalMouseY - setY));
				tresize = true;
			}
			else if (m_bottomLeftResize)		// bottom-left
			{
				int setX = m_pastX;

				// left resize
				if (m_pastGlobalMouseX < QCursor::pos().x()) {
					if (min_width <= m_pastWidth - margin_x) {
						setX = QCursor::pos().x();
					}
					else {
						setX = m_pastX + m_pastWidth - min_width;
					}
				}
				else if (m_pastGlobalMouseX > QCursor::pos().x()) {
					setX = QCursor::pos().x();
				}

				r = QRect(setX, m_pastY, m_pastWidth + (m_pastGlobalMouseX - setX), m_pastHeight - (m_pastMouseY - y) /* bottom resize */);
				tresize = true;
			}
			else if (m_bottomRightResize)
			{
				r = QRect(m_pastX, m_pastY, m_pastWidth - (m_pastMouseX - x) /* right resize */, m_pastHeight - (m_pastMouseY - y) /* bottom resize */);
				tresize = true;
			}

			if (tresize) {
				if (r.width() < min_width) r.setWidth(min_width);
				if (r.height() < min_height) r.setHeight(min_height);
				setGeometry(r);
				static_cast<QWidget*>(parent())->setGeometry(r);
			}
		}
		break;
	}
	case QEvent::MouseButtonPress:
	{
		m_pastMouseX = x;
		m_pastMouseY = y;

		m_pastX = static_cast<QWidget*>(parent())->x();
		m_pastY = static_cast<QWidget*>(parent())->y();

		m_pastWidth = static_cast<QWidget*>(parent())->width();
		m_pastHeight = static_cast<QWidget*>(parent())->height();

		m_pastGlobalMouseX = QCursor::pos().x();
		m_pastGlobalMouseY = QCursor::pos().y();

		if (m_enableMoveWidget) {
			if (y < m_titleHeight) {
				m_titleBarClicked = true;
			}
		}

		if (!m_enableResize) break;

		if (x < m_borderWidth && y > size.height() - m_borderWidth) {
			this->setCursor(QCursor(Qt::SizeBDiagCursor));
			m_bottomLeftResize = true;
		}
		else if (x > size.width() - m_borderWidth && y > size.height() - m_borderWidth) {
			this->setCursor(QCursor(Qt::SizeFDiagCursor));
			m_bottomRightResize = true;
		}
		else if (x < m_borderWidth && y < m_borderWidth) {
			this->setCursor(QCursor(Qt::SizeFDiagCursor));
			m_topLeftResize = true;
		}
		else if (x > size.width() - m_borderWidth && y < m_borderWidth) {
			this->setCursor(QCursor(Qt::SizeBDiagCursor));
			m_topRightResize = true;
		}
		else if (x < m_borderWidth) {
			this->setCursor(QCursor(Qt::SizeHorCursor));
			m_leftResize = true;
		}
		else if (y < m_borderWidth) {
			this->setCursor(QCursor(Qt::SizeVerCursor));
			m_topResize = true;
		}
		else if (y > size.height() - m_borderWidth) {
			this->setCursor(QCursor(Qt::SizeVerCursor));
			m_bottomResize = true;
		}
		else if (x > size.width() - m_borderWidth) {
			this->setCursor(QCursor(Qt::SizeHorCursor));
			m_rightResize = true;
		}
		else if (y < m_titleHeight) {
			m_titleBarClicked = true;
		}
		else {
			this->setCursor(QCursor(Qt::ArrowCursor));
			ReleaseMouse();
		}
		break;
	}
	case QEvent::Enter:
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
		break;
	case QEvent::Leave:
		this->setCursor(QCursor(Qt::ArrowCursor));
		break;
	case QEvent::WindowActivate:
	case QEvent::Paint:
	{
		RaiseChildWidget();
		break;
	}
	default:
		break;
	}

	return QDialog::event(event);
}

void dimmingWidget::RaiseChildWidget()
{
	if (m_child) {
		m_child->raise();
		m_child->activateWindow();
	}
}