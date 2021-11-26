#include "ViewerListPopUp.h"

#include <qsettings.h>
#include <qscrollbar.h>
#include <qlistwidget.h>
#include <qpainter.h>
#include <qurl.h>
#include <qnetworkaccessmanager.h>
#include <qnetworkreply.h>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsEffect>

#include <Windows.h>

SortByInsertTickItem::SortByInsertTickItem(QListWidget *parent)
	: QListWidgetItem(parent) {}

ViewerListPopUp::ViewerListPopUp(QWidget *parent)
	: QFramelessPopup(parent), mMainWindow(parent)
{
	ui.setupUi(this);
	ui.viewer_listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	ui.viewer_listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setAttribute(Qt::WA_TranslucentBackground);

	QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect(ui.main_widget);
	effect->setBlurRadius(10);
	effect->setColor(SHADOW_COLOR);
	effect->setOffset(QPointF(0, 10));
	ui.main_widget->setGraphicsEffect(effect);
}

ViewerListPopUp::~ViewerListPopUp()
{
}

void ViewerListPopUp::AddViewer(StreamRoomUserInfo& _info)
{
	for (int i = 0; i < ui.viewer_listWidget->count(); i++)
	{
		if (static_cast<SortByInsertTickItem*>(ui.viewer_listWidget->item(i))->GetGameUserId() == _info.gameUserId)
		{
			return;
		}
	}

	SortByInsertTickItem *item = new SortByInsertTickItem(ui.viewer_listWidget);
	item->SetInsertTick(std::chrono::system_clock::now());
	item->SetGameUserId(_info.gameUserId);
	ui.viewer_listWidget->addItem(item);
	ViewerListItem* theItem = new ViewerListItem();
	item->SetViewerWidget(theItem);
	item->setSizeHint(QSize(VIEWER_ITEM_WIDTH, VIEWER_ITEM_HEIGHT));
	ui.viewer_listWidget->setItemWidget(item, theItem);
	ui.viewer_listWidget->sortItems(Qt::AscendingOrder);
	theItem->SetUserInfo(_info);
	UpdateProfile(QString::fromLocal8Bit(_info.profileImageLarge.c_str()), QString::fromLocal8Bit(_info.gameUserId.c_str()));
}

int ViewerListPopUp::getViewerCount()
{
	return ui.viewer_listWidget->count();
}

bool ViewerListPopUp::RemoveViewer(string _id)
{
	for (int i = 0; i < ui.viewer_listWidget->count(); i++)
	{
		if (static_cast<SortByInsertTickItem*>(ui.viewer_listWidget->item(i))->GetGameUserId() == _id)
		{
			ui.viewer_listWidget->removeItemWidget(ui.viewer_listWidget->takeItem(i));
			break;
		}
	}

	static_cast<MainWindow*>(mMainWindow)->setViewerCount(getViewerCount());
	static_cast<MainWindow*>(mMainWindow)->ResetGeoViewerPopUp();

	return true;
}

int ViewerListPopUp::getViewPopUpHeight() {
	int returnVal = 0;
	int originalHeight = 0;
	int viewerCnt = ui.viewer_listWidget->count();
	int relative_max_height = LERATIVE_MAX_HEIGHT(mMainWindow->height());

	returnVal = CHARACTER_HEIGTH * viewerCnt + ((viewerCnt - 1) * CHARACTER_MARGIN) + TOP_BOTTOM_MARGIN + SCROLL_HIDE_MARGIN;
	originalHeight = returnVal;

	if (returnVal >= relative_max_height) {
		if (relative_max_height >= MAX_HEIGHT) {
			returnVal = MAX_HEIGHT;
		}
		else {
			returnVal = relative_max_height;
		}
	}
	else {
		if (returnVal > MAX_HEIGHT)
			returnVal = MAX_HEIGHT;
	}

	return returnVal;
}

void ViewerListPopUp::setPopUpSize(const int _width, const int _height)
{
	ui.main_widget->setFixedHeight(_height);
	ui.main_widget->setFixedWidth(_width);
	setFixedHeight(_height + SHADOW_MARGIN * 2);
	setFixedWidth(_width + SHADOW_MARGIN);
}

void ViewerListPopUp::UpdateProfile(QString _url, QString userId)
{
	QNetworkAccessManager *manager = new QNetworkAccessManager(this);
	connect(manager, &QNetworkAccessManager::finished, this, &ViewerListPopUp::ProfileImageDownloadFinished);
	connect(manager, &QNetworkAccessManager::finished, manager, &QNetworkAccessManager::deleteLater);

	const QUrl url = QUrl(_url);

	QNetworkRequest request(url);
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

	QNetworkReply* reply = manager->get(request);
	reply->setObjectName(userId);
}

void ViewerListPopUp::ProfileImageDownloadFinished(QNetworkReply *reply)
{
	QPixmap pm;
	if (!pm.loadFromData(reply->readAll())) {
		pm.load(":/StartStreamDlg/image/ic_default_character.png");
	}

	for (int i = 0; i < ui.viewer_listWidget->count(); i++)
	{
		SortByInsertTickItem* item = static_cast<SortByInsertTickItem*>(ui.viewer_listWidget->item(i));
		if (item->GetGameUserId() == reply->objectName().toStdString())
		{
			item->GetViewerWidget()->SetUserProfile(pm);
			item->GetViewerWidget()->update();
			break;
		}
	}
}