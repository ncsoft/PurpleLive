#pragma once

#include <qwidget.h>

#include "ui_GuildUserListItem.h"
#include "lime-defines.h"

#define GUILDUSER_IMAGE_SIZE			32
#define GUILDUSER_CHECK_SIZE			24
#define	GUILDUSER_IMAGE_BORDER_COLOR	Qt::transparent
#define	GUILDUSER_IMAGE_BORDER			0
#define SELECTED_ICON_COLOR				QColor("#731BFF")
#define OVER_ICON_COLOR					QColor(47, 53, 84, 255 * 0.08)

class GuildUserListItem : public QWidget
{
	Q_OBJECT

public:
	GuildUserListItem(QWidget *parent = Q_NULLPTR);
	~GuildUserListItem();
	void SetIndex(int index) { m_index = index; }
	void SetGuildUserInfo(GuildUserInfo& _gui);
	void SetSelected(bool select) { m_isSelected = select; }
	void SetClicked() { m_isSelected = !m_isSelected; }
	bool GetSelected() { return m_isSelected; }
	string GetUserProfileUrl() { return m_guildUserInfo.profileImageUrl; }
	void SetUserProfileImage(QPixmap image) { m_userProfile = image; }
	GuildUserInfo GetGuildUserInfo() { return m_guildUserInfo; }

private:
	Ui::GuildUserListItem ui;
	int m_index = -1;
	bool m_isSelected = false;
	bool m_isOvered = false;
	GuildUserInfo m_guildUserInfo;
	QPixmap m_userProfile;
	QPixmap m_defaultUserProfile;
	QPixmap m_selectImage;
	QPixmap m_selectOverImage;
	QColor m_selectedCheckColor = SELECTED_ICON_COLOR;
	QColor m_overCheckColor = OVER_ICON_COLOR;

private slots:
	bool event(QEvent *event);
	void paintEvent(QPaintEvent *e);
};