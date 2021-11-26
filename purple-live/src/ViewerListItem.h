#pragma once

#include <qwidget.h>

#include "lime-defines.h"
#include "ui_ViewerListItem.h"

#define		VIEWER_ITEM_WIDTH				224
#define		VIEWER_ITEM_HEIGHT				40
#define		VIEWER_IMAGE_SIZE				24
#define		VIEWER_DEPORT_IMAGE_SIZE		24
#define		VIEWER_IMAGE_BORDER_COLOR		Qt::transparent
#define		VIEWER_IMAGE_BORDER				0
#define		VIEWER_NAME_MAX_WIDTH			96

class ViewerListItem : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(QColor iconColor WRITE SetIconColor READ GetIconColor)

public:
	ViewerListItem(QWidget *parent = Q_NULLPTR);
	~ViewerListItem();

	void SetUserProfile(QPixmap profile) { m_userProfile = profile; }
	void SetUserInfo(StreamRoomUserInfo& _info);
	string GetUserId() { return m_roomUserInfo.gameUserId; }

private:
	Ui::ViewerListItem ui;

	StreamRoomUserInfo m_roomUserInfo;
	QPixmap m_deportBtnPixmap;
	QPixmap m_userEmptyProfile;
	QPixmap m_userProfile;

	// q-property
	QColor m_iconColor;

	void SetIconColor(QColor val);
	QColor GetIconColor() { return m_iconColor; }

private slots:
	void paintEvent(QPaintEvent *e);
	bool eventFilter(QObject* obj, QEvent* event);
	void on_deport_btn_clicked();
};