#include "MainWindowPushButtons.h"

#include <qpainter.h>
#include <qevent.h>
#include "AppUtils.h"

MainWindowSettingButton::MainWindowSettingButton(QWidget *parent)
	: QPushButton(parent)
{
}

void MainWindowSettingButton::paintEvent(QPaintEvent *e)
{
	QPushButton::paintEvent(e);

	QPainter painter;
	painter.begin(this);

	if (m_paintIcon.isNull() == false)
	{
		// Draw icon
		DrawUtils::DrawRectImage(&m_paintIcon, &painter, m_iconGeo);
	}

	painter.end();
}

void MainWindowSettingButton::SetNormalColor(QColor color)
{
	m_normalColor = color;
	m_paintIcon = DrawUtils::ChangeIconColor(m_paintIcon, m_normalColor);
}

void MainWindowSettingButton::showEvent(QShowEvent* e)
{
	m_iconGeo = { width() / 2 - DEFAULT_ICON_SIZE / 2, height() / 2 - DEFAULT_ICON_SIZE / 2, DEFAULT_ICON_SIZE, DEFAULT_ICON_SIZE };
}

void MainWindowSettingButton::changeEvent(QEvent *e)
{
	if (e->type() == QEvent::StyleChange)
		m_paintIcon = DrawUtils::ChangeIconColor(m_paintIcon, isEnabled() ? m_normalColor : m_disableColor);
}

MutePushButton::MutePushButton(QWidget *parent)
	: MainWindowSettingButton(parent)
{
	m_muteOff = QPixmap(":/MainWindow/image/ic_btn_mute_off.png");
	m_muteOn = QPixmap(":/MainWindow/image/ic_btn_mute_on.png");
}

void MutePushButton::SetMuted(bool muted, bool disable)
{
	if (disable)
	{
		m_muteOn = DrawUtils::ChangeIconColor(m_muteOn, m_disableColor);
		m_paintIcon = m_muteOn;
	}
	else
	{
		if (muted)
		{
			m_muteOn = DrawUtils::ChangeIconColor(m_muteOn, m_normalColor);
			m_paintIcon = m_muteOn;
		}
		else
		{
			m_muteOff = DrawUtils::ChangeIconColor(m_muteOff, m_normalColor);
			m_paintIcon = m_muteOff;
		}
	}

	setDisabled(disable);
	update();
}

ViewerPushButton::ViewerPushButton(QWidget *parent)
	: MainWindowSettingButton(parent)
{
	m_paintIcon = QPixmap(":/MainWindow/image/user.svg");
}

void ViewerPushButton::SetDisabled(bool disable)
{
	if (disable) m_paintIcon = DrawUtils::ChangeIconColor(m_paintIcon, m_disableColor);
	else m_paintIcon = DrawUtils::ChangeIconColor(m_paintIcon, m_normalColor);

	setDisabled(disable);
}

bool ViewerPushButton::SetViewerCount(int viewer)
{
	bool lagerThanMaxViewNum = false;
	if (viewer > VIEWER_MAXIMUM_NUM)
	{
		lagerThanMaxViewNum = true;
		viewer = VIEWER_MAXIMUM_NUM;
	}

	QString strViewer = QString::number(viewer);
	bool changedDigits = false;
	if (m_viewPrintNum.length() != strViewer.length())
	{
		changedDigits = true;
	}
	m_viewPrintNum = strViewer;

	// set disable button
	bool viewerButtonDisabled = true;
	if (viewer <= 0) {}
	else { viewerButtonDisabled = false; }
	SetDisabled(viewerButtonDisabled);

	// print string and reset width
	QString printString = QString::number(viewer);
	
	bool bToDoubleWorked;
	double dValue = printString.toDouble(&bToDoubleWorked);
	if (bToDoubleWorked)
	{
		printString = QString("%L1").arg(dValue, printString.length(), 'f', 0, ' ');
	}
	
	if(lagerThanMaxViewNum) printString += "+";

	int width = DrawUtils::GetFontWidth(AppUtils::GetMediumFont(12), printString);
	width += VIEWER_BTN_EXTRA_WIDTH;
	m_viewPrintNum = printString;
	setFixedWidth(width);

	if (changedDigits)
	{
		repaint();
		return true;
	}
	else
	{
		update();
		return false;
	}
}

void ViewerPushButton::paintEvent(QPaintEvent *e)
{
	MainWindowSettingButton::paintEvent(e);

	QPainter painter;
	painter.begin(this);

	// Draw Text
	painter.setPen(isEnabled() ? m_normalColor : m_disableColor);
	painter.setFont(AppUtils::GetBoldFont(VIEWER_BTN_FONT_SIZE));
	painter.drawText(
		VIEWER_BTN_IMAGE_X_MARGIN + SETTING_ICON_SIZE + 4,											// x
		height() / 2 - 20 / 2,																		// y
		DrawUtils::GetFontWidth(AppUtils::GetBoldFont(VIEWER_BTN_FONT_SIZE), m_viewPrintNum),		// width
		20,																							// height
		Qt::AlignLeft, m_viewPrintNum);
	
	painter.end();
}

void ViewerPushButton::showEvent(QShowEvent* e)
{
	SetIconGeo(QRect(VIEWER_BTN_IMAGE_X_MARGIN, height() / 2 - SETTING_ICON_SIZE / 2, SETTING_ICON_SIZE, SETTING_ICON_SIZE));
	m_paintIcon = DrawUtils::ChangeIconColor(m_paintIcon, isEnabled() ? m_normalColor : m_disableColor);
}
