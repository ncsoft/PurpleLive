#include "MuteCheckBox.h"
#include "AppUtils.h"

#include <qpainter.h>

MuteCheckBox::MuteCheckBox(QWidget *parent)
	: QCheckBox(parent)
{
	m_muteOnIcon = QPixmap(":/menu/image/menu/mute_on.svg");
	m_muteOffIcon = QPixmap(":/menu/image/menu/mute_off.svg");
}

void MuteCheckBox::paintEvent(QPaintEvent *e)
{
	QPainter painter;
	painter.begin(this);

	QPixmap* paintPixmap;

	if (isChecked() || !isEnabled())
	{
		paintPixmap = &m_muteOnIcon;
	}
	else
	{
		paintPixmap = &m_muteOffIcon;
	}

	DrawUtils::DrawRectImage(paintPixmap, &painter, QRect(0, 0, width(), height()));

	painter.end();
}

void MuteCheckBox::SetIconColor(QColor color)
{
	m_iconColor = color;
	m_muteOnIcon = DrawUtils::ChangeIconColor(m_muteOnIcon, m_iconColor);
	m_muteOffIcon = DrawUtils::ChangeIconColor(m_muteOffIcon, m_iconColor);
}