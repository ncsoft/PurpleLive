#include "GuildUserListItem.h"
#include "AppUtils.h"

#include <qpainter.h>

GuildUserListItem::GuildUserListItem(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	m_defaultUserProfile = QPixmap(":/StartStreamDlg/image/ic_default_character.png");
	m_selectImage = QPixmap(":/StartStreamDlg/image/ic_checked.svg");
	m_selectImage = DrawUtils::ChangeIconColor(m_selectImage, m_selectedCheckColor);
	m_selectOverImage = DrawUtils::ChangeIconColor(m_selectImage, m_overCheckColor);
}

GuildUserListItem::~GuildUserListItem()
{

}

void GuildUserListItem::SetGuildUserInfo(GuildUserInfo& _gui)
{
	m_guildUserInfo = _gui;
	ui.guildUserName_label->setText(QString::fromUtf8(m_guildUserInfo.characterName.c_str()));
}

void GuildUserListItem::paintEvent(QPaintEvent *e)
{
	QPainter painter(this);

	QRect _profileR(8, height() / 2 - GUILDUSER_IMAGE_SIZE / 2, GUILDUSER_IMAGE_SIZE, GUILDUSER_IMAGE_SIZE);

	if (m_userProfile.isNull() == false)
	{
		DrawUtils::DrawEllipseImage(&m_userProfile, &painter, _profileR);
	}
	else
	{
		DrawUtils::DrawEllipseImage(&m_defaultUserProfile, &painter, _profileR);
	}

	QRect _checkR(width() - 12 - GUILDUSER_CHECK_SIZE, height() / 2 - GUILDUSER_CHECK_SIZE / 2, GUILDUSER_CHECK_SIZE, GUILDUSER_CHECK_SIZE);

	if (m_isOvered)
	{
		DrawUtils::DrawRectImage(&m_selectOverImage, &painter, _checkR);
	}

	if (m_isSelected)
	{
		DrawUtils::DrawRectImage(&m_selectImage, &painter, _checkR);
	}

	painter.end();
}

bool GuildUserListItem::event(QEvent *event)
{
	switch (event->type())
	{
	case QEvent::Enter:
		m_isOvered = true;
		break;
	case QEvent::Leave:
		m_isOvered = false;
		break;
	}

	return QWidget::event(event);
}