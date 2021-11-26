#pragma once

#include <qwidget.h>

#include "lime-defines.h"
#include "QFramelessPopup.h"
#include "ui_ChatListItem.h"
#include "ui_ChatListPopUp.h"

#include <qurl.h>
#include <qnetworkaccessmanager.h>

#define CHECK_ICON_SIZE						24
#define CHECK_ICON_X_MARGIN					8
#define PROFILE_ICON_X_MARGIN				8
#define PROFILE_ICON_SIZE					32
#define PROFILE_RADIUS						10
#define CHATLIST_POPUP_ITEM_HEIGHT			32
#define CHATLIST_POPUP_MAX_HEIGHT			400
#define CHATLIST_POPUP_HEIGHT_MARGIN		16
#define CHATLIST_POPUP_WIDTH_MARGIN			60
#define CHAT_POPUP_ICON_SIZE				16
#define CHAT_NAME_LABEL_FONT_SIZE			14
#define CHAT_NAME_BTN_FONT_SIZE				12
#define CHAT_CHANNEL_BTN_WIDTH_MARGIN		30
#define CHAT_LABEL_MAX_WIDTH				(166+4+92)
#define CHAT_CHANNEL_BTN_MAX_WIDTH			(92-CHAT_CHANNEL_BTN_WIDTH_MARGIN)
#define CHAT_ROOM_NAME_MAX_WIDTH			166
#define SHADOW_BLUR_RADIUS					10
#define SHADOW_Y_OFFSET						10
#define SHADOW_COLOR						QColor(0, 0, 0, 255 * 0.08)

class ChatRoomInfo;
class ChatListItem;

class ChatListPopUpWidget : public QFramelessPopup
{
	Q_OBJECT
	Q_PROPERTY(QColor iconColor WRITE SetIconColor READ GetIconColor)

public:
	ChatListPopUpWidget(QWidget *parent = Q_NULLPTR, QWidget *itemParent = Q_NULLPTR);
	~ChatListPopUpWidget();

	void SetRoomInfo(vector<ChatRoomInfo*>& roomInfo);
	void SetGeometry(QRect rect);
	int GetRoomCount() { return ui.ChatListPopUpWidget->count(); }
	QListWidget* GetPopUpList() { return ui.ChatListPopUpWidget; }
	int GetMaximumWidth();
	int GetCurrentIndex() { return m_currentRow; }
	void SaveCurrentRow();

private:
	Ui::ChatListPopUp		ui;
	int						m_savedRow = -1;
	int						m_currentRow = 0;
	bool					m_isItemSelected = false;
	vector<QString>			m_vecRoomName{};
	QPixmap					m_headsetImage;
	QPixmap					m_chatImage;
	ChatListItem*			m_itemParentWidget = Q_NULLPTR;

	void ChangeIconColor();
	virtual const int getTitlebarHeight() { return INT_MIN; }
	virtual const int getTitlebarWidth() { return INT_MIN; }

	// q-property value
	QColor m_iconColor;

	void SetIconColor(QColor color);
	QColor GetIconColor() { return m_iconColor; }

public slots:
	void show();
	void hide();
	void itemClicked(QListWidgetItem* item);
};

class ChatListItem : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(QColor chatNameColor WRITE SetChatNameColor READ GetChatNameColor)
	Q_PROPERTY(QColor makeNewChatNameColor WRITE SetMakeNewChatNameColor READ GetMakeNewChatNameColor)
	Q_PROPERTY(QColor makeNewChaticonColor WRITE SetMakeNewChaticonColor READ GetMakeNewChaticonColor)
	Q_PROPERTY(QColor subchatBtnNormal WRITE SetSubchatBtnNormalColor READ GetSubchatBtnNormalColor)
	Q_PROPERTY(QColor subchatBtnActive WRITE SetSubchatBtnActiveColor READ GetSubchatBtnActiveColor)
	Q_PROPERTY(QColor subchatIconActive WRITE SetSubchatIconActiveColor READ GetSubchatIconActiveColor)
	Q_PROPERTY(QColor checkIconHover WRITE SetCheckIconHoverColor READ GetCheckIconHoverColor)
	Q_PROPERTY(QColor checkIconActive WRITE SetCheckIconActiveColor READ GetCheckIconActiveColor)

public:
	ChatListItem(QWidget *parent = Q_NULLPTR, int index = -1, bool isMakeNewChat = false);
	~ChatListItem();

	void SetGroupInfo(GroupInfo* gi);
	void SetIsOvered(bool over) { m_isOvered = over; }
	void SetSelected(bool selected);
	bool GetSelected() { return m_selected; }
	bool GetIsOvered() { return m_isOvered; }
	bool IsMakeNewChat() { return m_makeNewChat; }
	QString GetGroupUserID();
	QString GetChannelID();
	void SelectChannel(std::string groupId, std::string channelid);
	bool CheckHaveChannelID(const QString& channelid);
	void SetChatName(QString name);
	QString GetChatName();
	QString GetSubChatName();
	void SetChatListProfile(QPixmap& profile);
	void SetChatListProfileFail();
	QPixmap GetChatListProfile();
	void SetBtnText(QString& text);
	void ResetSelect(bool isSelected);
	std::shared_ptr<ChatListPopUpWidget> GetChatListPopUp();
	void ShowSubChatPopUp(int _x, int _y);
	vector<ChatRoomInfo*> GetRoomInfo() { return m_vecChatRoomInfo; }

private slots:
	void UpdateLanguageString();
	void on_changeSubChat_btn_clicked();
	bool event(QEvent *event);
	bool eventFilter(QObject* obj, QEvent* event);

private:
	void paintEvent(QPaintEvent *e);
	void HidePopUpWidget();
	void MakePopUpWidget();
	void UpdateProfileImage(string url);
	void ProfileImageDownloadFinished(QNetworkReply *reply);
	
	Ui::ChatListItem ui;

	int m_index;
	QPixmap m_chatProfile;
	QPixmap m_selectedCheck;
	QPixmap m_overCheck;
	QPixmap m_newChatProfile;
	QPixmap m_defaultProfile;
	QPixmap m_arrowDown;
	QPixmap m_arrowUpActive;
	QFont m_chatNameFont;
	QFont m_subChatBtnFont;
	QColor m_chatNameColor;
	QColor m_makeNewChatNameColor;
	QString m_originChatRoomName{};
	QString m_printChatRoomName{};
	QString m_gourpId{};
	QString m_subchatPrintStr{};
	int m_subchatFontWidth = 0;
	int m_chatNameWidth = 0;
	bool m_isMoveCalled = false;
	bool m_makeNewChat = false;
	bool m_profileDownloaded = false;
	bool m_selected = false;
	bool m_isOvered = false;
	ChatRoomInfo* m_chatRoomInfo;
	vector<ChatRoomInfo*> m_vecChatRoomInfo{};
	std::shared_ptr<ChatListPopUpWidget> m_chatListPopup;

	// q-property values
	QColor m_subchatBtnNormal;
	QColor m_subchatBtnActive;
	QColor m_subchatIconActive;
	QColor m_checkIconHover;
	QColor m_checkIconActive;
	QColor m_makeNewChaticonColor;

	void SetChatNameColor(QColor color) { m_chatNameColor = color; }
	QColor GetChatNameColor() { return m_chatNameColor; }
	void SetMakeNewChatNameColor(QColor color) { m_makeNewChatNameColor = color; }
	QColor GetMakeNewChatNameColor() { return m_makeNewChatNameColor; }

	void SetSubchatBtnNormalColor(QColor color);
	QColor GetSubchatBtnNormalColor() { return m_subchatBtnNormal; }
	void SetSubchatBtnActiveColor(QColor color);
	QColor GetSubchatBtnActiveColor() { return m_subchatBtnActive; }
	void SetSubchatIconActiveColor(QColor color);
	QColor GetSubchatIconActiveColor() { return m_subchatIconActive; }

	void SetCheckIconHoverColor(QColor color);
	QColor GetCheckIconHoverColor() { return m_checkIconHover; }
	void SetCheckIconActiveColor(QColor color);
	QColor GetCheckIconActiveColor() { return m_checkIconActive; }

	void SetMakeNewChaticonColor(QColor color);
	QColor GetMakeNewChaticonColor() { return m_makeNewChaticonColor; }
};