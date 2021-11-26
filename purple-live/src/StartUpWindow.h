#pragma once

#include <QDialog>
#include <QStyledItemDelegate>
#include <qnetworkreply.h>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsEffect>
#include <qscopedpointer.h>

#include "ui_StartUpWindow.h"
#include "QFramelessDialog.h"
#include "ChatListItem.h"
#include "CharacterListItem.h"
#include "CharacterSelectWidget.h"
#include "MicSettingWidget.h"
#include "MicSettingListItem.h"
#include "MaskSettingListItem.h"
#include "MaskSettingWidget.h"
#include "WebViewWidget.h"
#include "GuildUserListItem.h"
#include "AudioMixer/NCAudioMixer.h"
#include "DimWidget.h"

class ComboBoxItem;

#define		STARTUP_WIDGET_WIDTH			392							// px
#define		STARTUP_WIDGET_HEIGHT_MAX		640
#define		STARTUP_WIDGET_HEIGHT_MIN		464
#define		STARTUP_LIST_ITEM_HEIGHT		48
#define		STARTUP_LIST_TOP_BOTTOM_MARGIN	296
#define		STARTUP_LIST_HEIGHT_MIN			172
#define		STARTUP_LIST_GUILD_TOP_MARGIN	40

#define		CHARACTER_IMAGE_SIZE			32
#define		CHARACTER_IMAGE_BORDER			0
#define		CHARACTER_IMAGE_BORDER_COLOR	Qt::transparent // erase border

#define		CHATLIST_ITEM_HEIGHT			48
#define		CHATLIST_ITEM_WIDTH				318
#define		CHATLIST_ITEM_X_MARGIN			5

#define		MICLIST_ITEM_HEIGHT				40
#define		MICLIST_ITEM_WIDTH				318

#define		SELECTED_CHATLIST_POPUP_WIDTH	313
#define		SELECTED_CHATLIST_POPUP_HEIGHT	48

#define		MASK_SET_ITEM_HEIGHT			32
#define		MASK_SET_ITEM_WIDTH				280

#define		LISTWIDGET_X_MARGN				16

struct ChatRoomInfo {
	QString labelName;					// label name
	QString gameChannelType;			// GUILD, ONE_ON_ONE
	QString chatRoomName;				// room name
	QString groupUserId;
	QString channelId;
	bool bLabelTitle;
	bool bVoiceRoom;
};

class StartUpWindow : public QFramelessDialog
{
	Q_OBJECT
	Q_PROPERTY(QColor emptyChatroomMsgColor WRITE SetEmptyChatroomMsgColor READ GetEmptyChatroomMsgColor)

public:
	StartUpWindow(QWidget *parent = Q_NULLPTR);
	~StartUpWindow();

	void Show();
	bool Init();
	void InitInfo();
	bool CheckServiceInspection();
	bool GetCharacterList();
	bool ConnectPubServer();
	int GetCharacterIndex();
	void SetCharacterIndex(int index);
	void InitCharactor();
	void ReloadCharactor();

	bool GetActive();
	void SetActive(bool enable);
	void SetMicList();
	int GetUserCharacCount();
	void DisableAllButtons(bool disable);
	void StartStreaming(std::string& groupUserId, std::string& channelId);
	bool CheckStartingStream() { return m_startingStream; }
	virtual const int getTitlebarHeight() { return ui.widget_title->height(); }
	virtual const int getTitlebarWidth() { return ui.widget_title->width(); }
	
	// public Chatlist ListView Func
	void HideChatListItemPopUp();							/* Hide all ChatList PopUp */
	
	// public Character Select Widget Func
	void OnCompleteChangeCharacter();
	void OnCreateChatting(bool success);
	void OnGetGuildUserList(bool success);
	void OnGetShareTargetList(bool success);
	void OnLanguageChanged();

	bool ReloadGuildUser();
	int GetCharacterListHeight();
	bool GetCharacterSelectWidgetIsHidden();
	QPixmap* GetUserImageFromIndex(int index);
	void SelectedCharacterChange(int index);
	void OnChangeCharactor(int index);
	void ProcessChangeCharacter(int index);

	// public MicSetting Widget Func
	void SetMicSelection(int index);
	QString GetMicDeviceID(int index);

	// public MaskSetting Widget Func
	void SaveMaskInfotoLocal(bool save);
	int GetMaskItemCount() { return m_maskSettingWidget->GetListWidget()->count(); }

	// publid Guild User List Func
	GuildUserListItem* GetGuildUserListItem(int index);
	void SelectAllGuildUser(bool bselectAll);
	int GetGuildUserCount() { return ui.shareWithGroup_widget->GetListWidget()->count(); }
	void SetStreamBtnDisable(bool disable) { ui.startstream_btn->setDisabled(disable); }
	bool GetStreamBtnDisable() { return !(ui.startstream_btn->isEnabled()); }

	void SetShowWebviewWidget(bool show);
	void SetWebviewUrl(QString url, bool forced);
	QString GetWebviewCharacterId();
	QString GetWebviewCharacterInfo();
	void SetWebviewCharacterInfo();
	void CreatedChattingChannel(const QString& channelid);

	// Dimming Circle Indicator
	void SetShowLoading(bool show);
	void ShowLoading(QWidget* child = Q_NULLPTR);		// call this func. when paint dim-widget
	void HideLoading();									// must, call this func. when paint end dim-widget
	bool IsDimmingShow();
	void UpdateDimmingRect();
	void ShowDimmingIndicator();

signals:
	void ResetGeometry();

private:
	Ui::StartUpWindow ui;

	int m_dimmingCnt = 0;
	bool m_startingStream = false;
	bool m_active = false;
	bool m_changedSizeOrPos = false;
	QString m_newChatChannelId{};
	QColor m_emptyChatroomMsgColor{};
	vector<QWidget*> m_vecCheckboxWidget;
	vector<AudioDeviceInfo> m_micInfoList;
	GuildUserInfoList m_selectedGroupInfo{};
	GameUserInfo m_currentSelectedUserInfo{};
	QWebEngineView* m_mapLoginWebView;
	QScopedPointer<CharacterSelectWidget> m_characterSelectWidget;
	QScopedPointer<MicSettingWidget> m_micSettingWidget;
	QScopedPointer<MaskSettingWidget> m_maskSettingWidget;
	QScopedPointer<WebViewWidget> m_webViewWidget;
	QScopedPointer<QGraphicsDropShadowEffect> m_showDropdownShadowEffect;
	QScopedPointer<dimmingPainter> m_dimmingPainter;

	bool nativeEvent(const QByteArray &eventType, void *message, long *result);
	void contextMenuEvent(QContextMenuEvent *event) override;
	void InitUI();
	void InitUIAfterCreateChild();
	void ShowSharedWithGroupWidget(bool bshow);
	void SetGameUserInfoList();
	void SetLocalSavedMicList();
	bool SetLocalSavedMaskList();
	bool VerifyArgument();
	void InitMAPLog();
	void ReloadChatList();

	// private ChatList ListView Func
	void ResetChatListItemSelect();
	int SortChatListItem(vector<GroupInfo>& group_info);
	void AddChatListItem(GroupInfo* gi, int index);
	int GetChatListSelectedRow();
	void SetSelectNewChatListItem();
	QString GetCurrentSelectedGroupUserID();
	QString GetCurrentSelectedChannelID();

	// private Charater Selection Widget Func
	void AddCharacterListItem(int index);
	void UpdateProfileImage(int index);
	void ResetHeight();

	// private MicSetting Widget Func
	void AddMicListItem(int index, AudioDeviceInfo* info = nullptr);
	bool GetMicIsDefault(int index);

	// private MaskSetting Widget Func
	void AddMaskListItem(int index, QString name, bool enable);

	// publid Guild User List Func
	void ClearGuildUserInfo();
	GuildUserInfoList& GetSelectedGroupList();

public slots:
	void ChatListItemRowChanged(int row);
	void ChatListItemActive(QListWidgetItem *item);
	void UpdateLanguageString();

private slots:
	void ProfileImageDownloadFinished(QNetworkReply *reply);
	void on_startstream_btn_clicked();
	void on_sharedWithChat_btn_clicked();
	void on_sharedWithGroup_btn_clicked();
	void on_close_btn_clicked();
	void on_min_btn_clicked();
	void on_mask_btn_clicked();
	void on_characterSelect_btn_clicked();
	void on_micsetting_btn_clicked();
	void OnUrlChanged(const QUrl &url);

	bool event(QEvent *event);
	bool eventFilter(QObject* obj, QEvent* event);
	void closeEvent(QCloseEvent * event);

	void SetEmptyChatroomMsgColor(QColor color) { m_emptyChatroomMsgColor = color; }
	QColor GetEmptyChatroomMsgColor() { return m_emptyChatroomMsgColor; }
};