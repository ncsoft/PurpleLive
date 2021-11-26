#pragma once

#include <qwidget.h>
#include "AppConfig.h"
#include "ui_ViewerListPopUp.h"
#include <qurl.h>
#include <chrono>

#include "MainWindow.h"
#include "QFramelessPopup.h"
#include "ViewerListItem.h"
#include "lime-defines.h"

#define		CHARACTER_LIST_POPUP_WIDTH		224			// px, width(고정 값)
#define		CHARACTER_HEIGTH				40			// px, 한 캐릭당 heigth
#define		CHARACTER_MARGIN				0			// px, 캐릭간 마진
#define		MAX_HEIGHT						400			// px, 최대 높이
#define		LERATIVE_MAX_HEIGHT(x)			x * 0.6		// px, 상대적 최대 높이
#define		TOP_BOTTOM_MARGIN				(8 + 8)	// px, top, bottom 마진
#define		SCROLL_HIDE_MARGIN				10			//
#define		SHADOW_MARGIN					12
#define		VIEWER_Y_MARGIN					6
#define		CHARACTER_IMAGE_BORDER_COLOR	Qt::transparent
#define		CHARACTER_IMAGE_BORDER			0

class SortByInsertTickItem : public QListWidgetItem
{
public:
	SortByInsertTickItem(QListWidget *parent = Q_NULLPTR);
	~SortByInsertTickItem() {}

	void SetInsertTick(std::chrono::system_clock::time_point tick) { m_insertedTick = tick; }
	void SetGameUserId(std::string _gameUserId) { m_gameUserid = _gameUserId; }
	void SetViewerWidget(ViewerListItem* ptr) { m_viewerWidget = ptr; }

	std::string GetGameUserId() { return m_gameUserid; }
	ViewerListItem* GetViewerWidget() { return m_viewerWidget; }

private:
	virtual bool operator <(const QListWidgetItem& other) const
	{
		return this->m_insertedTick < static_cast<const SortByInsertTickItem&>(other).m_insertedTick;
	}

	std::chrono::system_clock::time_point m_insertedTick;
	std::string m_gameUserid;
	ViewerListItem* m_viewerWidget = nullptr;
};

class ViewerListPopUp : public QFramelessPopup
{
	Q_OBJECT

public:
	ViewerListPopUp(QWidget *parent = Q_NULLPTR);
	~ViewerListPopUp();

	void setPopUpSize(const int _width, const int _height);
	QObject* getListWidget() { return static_cast<QObject*>(ui.viewer_listWidget); }
	void AddViewer(StreamRoomUserInfo& _info);
	bool RemoveViewer(string _id);

	int GetYMargin() { return VIEWER_Y_MARGIN; }
	int getViewerCount();
	int getViewPopUpHeight();
	int getViewPopUpWidth() { return CHARACTER_LIST_POPUP_WIDTH; }

private:
	Ui::ViewerListPopUp ui;

	QWidget* mMainWindow;

	void UpdateProfile(QString _url, QString userId);
	virtual const int getTitlebarHeight() { return 0; }
	virtual const int getTitlebarWidth() { return 0; }

private slots:
	void ProfileImageDownloadFinished(QNetworkReply *reply);
};