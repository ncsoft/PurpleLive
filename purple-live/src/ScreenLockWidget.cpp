#include "ScreenLockWidget.h"
#include "VideoChatApp.h"
#include "AppString.h"
#include "AppUtils.h"

#include <Windows.h>

#include <qevent.h>
#include <qpainter.h>

ScreenLockWidget::ScreenLockWidget(QWidget *parent) : QWidget(parent)
{
	connect(MainWindow::Get(), &MainWindow::UpdateChildLanguageString, this, &ScreenLockWidget::UpdateLanguageString);

	// Make Button
	m_screenlockBtn = new QPushButton(this);
	m_screenlockBtn->installEventFilter(this);
	m_screenlockBtn->show();
	m_screenlockBtn->setText(AppString::GetQString("ScreenLock/screen_lock_btn"));
}

QPointer<ScreenLockWidget> ScreenLockWidget::Instance()
{
	static QPointer<ScreenLockWidget> instance = new ScreenLockWidget;

	if (instance.isNull()) instance = new ScreenLockWidget(MainWindow::GetActiveWidget());
	else instance->setParent(MainWindow::GetActiveWidget());

	if (MainWindow::GetActiveWidget() == MainWindow::Get()) {
		connect(MainWindow::Get(), &MainWindow::ResetChildGeometry, instance, &ScreenLockWidget::ResetGeometry);
	} else {
		connect(MainWindow::Get()->GetStartupWindow(), &StartUpWindow::ResetGeometry, instance, &ScreenLockWidget::ResetGeometry);
	}

	return instance;
}

void ScreenLockWidget::ResetGeometry()
{
	QRect r = static_cast<QWidget*>(parent())->rect();
	setGeometry(0, 0, r.width(), r.height());

	m_iconToButtonHeight = m_iconToButtonMargin + m_iconSize.height() + m_buttonSize.height();
	m_iconToButtonHeight /= 2;

	int newX = width() / 2 - m_screenlockBtn->width() / 2;
	int newY = height() / 2 - m_iconToButtonHeight + m_iconSize.height() + m_iconToButtonMargin;
	m_screenlockBtn->setGeometry(newX, newY, m_screenlockBtn->width(), m_screenlockBtn->height());
}

void ScreenLockWidget::UpdateLanguageString()
{
	m_screenlockBtn->setText(AppString::GetQString("ScreenLock/screen_lock_btn"));
	update();
}

void ScreenLockWidget::Show()
{
	ActiveWidget(true);
}


void ScreenLockWidget::Hide()
{
	ActiveWidget(false);
}

void ScreenLockWidget::ActiveWidget(bool active)
{
	if (active)
	{
		emit SendScreenLockShowEvent();
		show();
	}
	else
	{
		emit SendScreenLockHidedEvent();
		hide();
	}

	if (parent() == MainWindow::Get()) static_cast<MainWindow*>(parent())->SetEnableMaximize(!active);
	MainWindow::Get()->BringToFront();
}

void ScreenLockWidget::showEvent(QShowEvent* e)
{
	ResetGeometry();
}

void ScreenLockWidget::closeEvent(QCloseEvent* e)
{
	delete ScreenLockWidget::Instance();
}

void ScreenLockWidget::paintEvent(QPaintEvent * e)
{
	QPainter painter(this);

	painter.setBrush(QBrush(m_backgroundColor));
	painter.setPen(QPen(m_backgroundColor));
	painter.drawRect(0, 0, width(), height());

	int posx = (width() - m_iconSize.width()) / 2;
	int posy = height() / 2 - m_iconToButtonHeight;//(height() - m_iconSize.height()) / 2;
	DrawUtils::DrawRectImage(&m_pixmapIcon, &painter, QRect(posx, posy, m_iconSize.width(), m_iconSize.height()));

	painter.end();
	
	QWidget::paintEvent(e);
}

void ScreenLockWidget::SetButtonSize(QSize val)
{
	m_buttonSize = val;
	m_screenlockBtn->setFixedHeight(m_buttonSize.height());
	m_screenlockBtn->setFixedWidth(m_buttonSize.width());
}

bool ScreenLockWidget::eventFilter(QObject* obj, QEvent* event)
{
	if (obj == m_screenlockBtn)
	{
		if (event->type() == QEvent::MouseButtonRelease)
		{
			App()->SendScreenUnlockToAgent();
		}
	}

	return QWidget::eventFilter(obj, event);
}

bool ScreenLockWidget::IsActive()
{
	if (isVisible()) {
		return true;
	}
	else {
		return false;
	}
}