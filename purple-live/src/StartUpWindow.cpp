#include "StartUpWindow.h"
#include "VideoChatApp.h"
#include "AppUtils.h"
#include "AppString.h"

#include <qsettings.h>
#include <qstandardpaths.h>
#include <qurl.h>
#include <qnetworkaccessmanager.h>
#include <qpainter.h>
#include <qscreen.h>
#include <qwindow.h>
#include <qscrollbar.h>
#include <qurlquery.h>

#include "AppString.h"
#include "LocalSettings.h"
#include "HiddenSettings.h"
#include "VideoChatMessageBox.h"
#include "LimeCommunicator.h"
#include "MaskingManager.h"
#include "MenuControl.h"
#include "webview/WebviewControl.h"
#include "webview/WebCommunicator.h"
#include "ServiceInspectionManager.h"
#include "MAPManager.h"
#include <json.h>

StartUpWindow::StartUpWindow(QWidget *parent)
	: QFramelessDialog(parent, true, 5, false)
{
	SetForegroundWindow((HWND)winId());

	ui.setupUi(this);

	InitUI();
}

StartUpWindow::~StartUpWindow()
{
}

bool StartUpWindow::GetActive()
{
	return m_active;
}

void StartUpWindow::SetActive(bool enable)
{
	m_active = enable;
	if (enable)
	{
		Show();
	}
	else
	{
		hide();
	}
} 

void StartUpWindow::AddMaskListItem(int index, QString name, bool enable)
{
	if (m_maskSettingWidget)
	{
		QListWidgetItem *item = new QListWidgetItem(m_maskSettingWidget->GetListWidget());
		m_maskSettingWidget->GetListWidget()->addItem(item);
		MaskSettingListItem* theItem = new MaskSettingListItem(this);
		item->setSizeHint(QSize(MASK_SET_ITEM_WIDTH, MASK_SET_ITEM_HEIGHT));
		m_maskSettingWidget->GetListWidget()->setItemWidget(item, theItem);
		theItem->SetIndex(index);
		theItem->SetMaskName(name);
		theItem->SetMaskChecked(enable);
	}
}

void StartUpWindow::SaveMaskInfotoLocal(bool save)
{
	m_maskSettingWidget->SaveLocal(save);

	for (int i = 0; i < m_maskSettingWidget->GetListWidget()->count(); i++)
	{
		QListWidgetItem *item = m_maskSettingWidget->GetListWidget()->item(i);
		MaskSettingListItem* itemWidget = static_cast<MaskSettingListItem*>(m_maskSettingWidget->GetListWidget()->itemWidget(item));

		itemWidget->SaveLocal(save);
	}

	bool is_enable_game_mask = LocalSettings::GetEnableGameMask(LocalSettings::DEFAULT_ENABLE_GAME_MASK);
	if (is_enable_game_mask)
	{
		ui.mask_btn->SetInfoName(AppString::GetQString("StreamDlg/mask_info_on"));
	}
	else
	{
		ui.mask_btn->SetInfoName(AppString::GetQString("StreamDlg/mask_info_off"));
	}
}

bool StartUpWindow::SetLocalSavedMaskList()
{
	MaskingManager::Instance()->Setup(App()->GetOriginGameCode());

	vector<wstring> vecMaskingArea;
	MaskingManager::Instance()->GetMaskNameList(vecMaskingArea);

	bool is_enable_game_mask = LocalSettings::GetEnableGameMask(LocalSettings::DEFAULT_ENABLE_GAME_MASK);
	ui.mask_btn->SetInfoName(AppString::GetQString(is_enable_game_mask ? "StreamDlg/mask_info_on" : "StreamDlg/mask_info_off"));

	// update masking list
	m_maskSettingWidget->GetListWidget()->clear();
	QBitArray game_mask_settings = LocalSettings::GetGameMaskSettings(App()->GetOriginGameCode(), QBitArray(vecMaskingArea.size()));
	for (int i = 0; i < vecMaskingArea.size(); i++) {
		AddMaskListItem(i, QString::fromStdWString(vecMaskingArea[i]), game_mask_settings.at(i));
	}

	m_maskSettingWidget->SetInitMaskEnable(is_enable_game_mask);
	m_maskSettingWidget->SetMaskItemCount(vecMaskingArea.size());

	if (m_maskSettingWidget->GetListWidget()->count() <= 0) {
		AppUtils::DispatchToMainThread([]() {
			MainWindow::Get()->AppClose(AppString::GetString("error_message/error_cannot_find_mask_info").c_str());
		});
		return false;
	}

	return true;
}

bool StartUpWindow::Init()
{
	m_characterSelectWidget.reset(new CharacterSelectWidget(this));
	m_micSettingWidget.reset(new MicSettingWidget(this));
	m_maskSettingWidget.reset(new MaskSettingWidget(this));
	m_webViewWidget.reset(new WebViewWidget(this));

	SetMicList();
	SetLocalSavedMicList();
	InitUIAfterCreateChild();

	SetActive(true);

	ShowLoading();

	AppUtils::DispatchToMainThread([this]() {
		InitInfo();
	});

	return true;
}

void StartUpWindow::InitInfo()
{
	if (!VerifyArgument())
		return;

//	if (false == CheckServiceInspection())
//		return;

	if (!GetCharacterList())
		return;

	InitCharactor();

	ConnectPubServer();

	InitMAPLog();
}

void StartUpWindow::InitMAPLog()
{
#if USE_MAP_LOG_DEBUG
	QUrl url = LOGIN_RC;

	url.setPath("/login/signin");
	QUrlQuery requestQuery;
	requestQuery.addQueryItem("target_app_id", AppConfig::APP_ID);
	requestQuery.addQueryItem("credential_result", "token");
	url.setQuery(requestQuery);

	QString str = url.toString();
	m_mapLoginWebView = new QWebEngineView(this);
	m_mapLoginWebView->show();
	m_mapLoginWebView->setGeometry(QRect(0, getTitlebarHeight(), width(), height() - getTitlebarHeight()));
	m_mapLoginWebView->load(url);

	connect(m_mapLoginWebView, &QWebEngineView::urlChanged, this, &StartUpWindow::OnUrlChanged);
#else
	MAPManager::Instance();
#endif//USE_MAP_LOG_DEBUG
}

void StartUpWindow::OnUrlChanged(const QUrl &url)
{
	QUrlQuery query(url);

	if (query.hasQueryItem("authn_token"))
	{
		MAPManager::Instance()->LogIn(query.queryItemValue("authn_token").toStdString(), App()->GetGameCode(), MAPManager::Instance()->MakeUUID(), App()->GetAppVersion());
		m_mapLoginWebView->close();
	}
}

bool StartUpWindow::CheckServiceInspection()
{
	string service_inspection_msg;
	if (!ServiceInspectionManager::Instance()->IsInspection(App()->GetServiceNetwork(), service_inspection_msg))
	{
		MainWindow::Get()->AppClose(AppString::GetString("error_message/error_service_inspection_server_connect_fail").c_str());
		return false;
	}

	Json::Reader reader;
	Json::Value root;
	if (false == reader.parse(service_inspection_msg, root))
	{
		MainWindow::Get()->AppClose(AppString::GetString("error_message/error_service_inspection_server_connect_fail").c_str());
		return false;
	}

	if (root.isMember("service_status") == false)
	{
		MainWindow::Get()->AppClose(AppString::GetString("error_message/error_service_inspection_server_connect_fail").c_str());
		return false;
	}

	int service_status = root["service_status"].asInt();
	// 1: 접속가능
	// 2: 점검
	// 3: 강제 업데이트
	// 4: 선택 업데이트
	if (service_status == 2)
	{
		string message, start_time, end_time, landing_url;
		if(root.isMember("message"))
			message = root["message"].asString();
		if (root.isMember("landing_url"))
			landing_url = root["landing_url"].asString();
		if (root.isMember("effective_from"))
		{
			string time = root["effective_from"].asString();
			int y, M, d, h, m;
			float s;
			sscanf(time.c_str(), "%d-%d-%dT%d:%d:%fZ", &y, &M, &d, &h, &m, &s);
			start_time = AppUtils::format("%d-%d-%d %d:%d", y, M, d, h, m);
		}
			
		if (root.isMember("effective_to"))
		{
			string time = root["effective_to"].asString();
			int y, M, d, h, m;
			float s;
			sscanf(time.c_str(), "%d-%d-%dT%d:%d:%fZ", &y, &M, &d, &h, &m, &s);
			end_time = AppUtils::format("%d-%d-%d %d:%d", y, M, d, h, m);
		}

		string inspection_msg = AppUtils::format("%s\n\n%s\n%s ~ %s", message.c_str(), 
			AppString::GetString("Common/service_inspection_time").c_str(),
			start_time.c_str(), end_time.c_str());
		MainWindow::Get()->AppClose(inspection_msg.c_str());

		return false;
	}
	else
	{
		return true;
	}

	return true;
}

bool StartUpWindow::VerifyArgument()
{
	// check App StandAlone
	if (!(App()->GetStandAlone())) {
		MainWindow::Get()->AppClose(AppString::GetString("error_message/error_stand_alone").c_str());
		return false;
	}

	// Set Language
	if (!App()->SetLocalLanguage(QString::fromStdString(App()->GetArgLanguage())))
	{
		LOG(ERROR) << format_string("[StartUpWindow::VerifyArgument] invalid language(%s)", App()->GetArgLanguage().c_str());
		App()->SetLocalLanguage(App()->GetLocalLanguage());
	}

	if (App()->GetGameHwndString().empty())
	{
		MainWindow::Get()->AppClose(AppString::GetString("error_message/error_argument_gamehwnd").c_str());
		return false;
	}

	return true;
}

bool StartUpWindow::GetCharacterList()
{
	return LimeCommunicator::Instance()->GetCharactorList(App()->GetCharacterCode().c_str(), App()->GetToken().c_str(), App()->GetServiceNetwork(), App()->GetAppVersion().c_str());
}

bool StartUpWindow::ConnectPubServer()
{
	LimeCommunicator::Instance()->Logout();
	string gameUserID = LimeCommunicator::Instance()->GetGameUserID(App()->GetCharacterName().c_str());
	return LimeCommunicator::Instance()->ConnectPubServer(gameUserID.c_str(), App()->GetToken().c_str(), App()->GetDeviceId().c_str(), App()->GetServiceNetwork(), App()->GetAppVersion().c_str());
}

int StartUpWindow::GetCharacterIndex()
{
	return LimeCommunicator::Instance()->GetCurrentCharacterIndex();
}

void StartUpWindow::SetCharacterIndex(int index)
{
	// Set Stream Character
	LimeCommunicator::Instance()->SetCurrentCharacterIndex(index);
	std::string name = LimeCommunicator::Instance()->GetCharacterNamefromIndex(index);
	App()->SetCharacterName(StringUtils::GetStringFromUtf8(name));

	// ClearLimeToken for loginWithPurple per character
	LimeCommunicator::Instance()->ClearLimeToken();
}

void StartUpWindow::InitCharactor()
{
	SetGameUserInfoList();

	int current_charactor_index = LimeCommunicator::Instance()->GetCharacterIndex(App()->GetCharacterName().c_str());
	if (current_charactor_index < 0) current_charactor_index = 0;
	LimeCommunicator::Instance()->SetCurrentCharacterIndex(current_charactor_index);
	SetCharacterIndex(current_charactor_index);
}

void StartUpWindow::ReloadCharactor()
{
	int index = GetCharacterIndex();
	SelectedCharacterChange(index);
	ProcessChangeCharacter(index);
}


bool StartUpWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
#if (QT_VERSION == QT_VERSION_CHECK(5, 11, 1))
	MSG* msg = *reinterpret_cast<MSG**>(message);
#else
	MSG* msg = reinterpret_cast<MSG*>(message);
#endif

	switch (msg->message)
	{
	case WM_MOVE:
	case WM_SIZING:
		m_changedSizeOrPos = true;
		break;
		
	}
	return QFramelessDialog::nativeEvent(eventType, message, result);
}

void StartUpWindow::contextMenuEvent(QContextMenuEvent *event)
{
	
}

bool StartUpWindow::event(QEvent *event)
{
	switch (event->type())
	{
		case QEvent::MouseButtonPress:
		{
			HideChatListItemPopUp();
			break;
		}
		case QEvent::MouseButtonRelease:
		{
			if (m_changedSizeOrPos)
			{
				m_changedSizeOrPos = false;
				LocalSettings::SetWindowGeometry(metaObject()->className(), geometry());
			}
			break;
		}
		case QEvent::Move:
		{
			emit ResetGeometry();
		}
	}
	
	return QFramelessDialog::event(event);
}

void StartUpWindow::HideChatListItemPopUp()
{
	if (ui.chat_listView->count() <= 0) return;

	for (int i = 0; i < ui.chat_listView->count() - 1; i++)
	{
		QListWidgetItem* item = ui.chat_listView->item(i);
		ChatListItem* itemWidget = static_cast<ChatListItem*>(ui.chat_listView->itemWidget(item));

		if (itemWidget->GetChatListPopUp() != nullptr) {
			if (itemWidget->GetChatListPopUp()->isHidden() == false)
				itemWidget->GetChatListPopUp()->hide();
		}
	}
}

bool StartUpWindow::eventFilter(QObject* obj, QEvent* event)
{
	if (ui.chat_listView->viewport() == obj)
	{
		if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick)
		{
			HideChatListItemPopUp();
		}
		else if (event->type() == QEvent::Paint)
		{
			if (ui.chat_listView->count() <= 1) {
				QPainter painter(ui.chat_listView->viewport());

				painter.setPen(m_emptyChatroomMsgColor);
				painter.setFont(AppUtils::GetMediumFont(13));
				painter.drawText(ui.StartUpW_vWidget->x() - ui.chat_listView->x() - 10,
					ui.chat_listView->viewport()->height() / 2 - 40 / 2,
					width(),
					40, Qt::AlignHCenter, AppString::GetQString("StreamDlg/empty_chatroom"));

				painter.end();
			}
		}
		else if (event->type() == QEvent::Wheel)
		{
			// when wheel event called over icon erase
			for (int i = 0; i < ui.chat_listView->count() - 1; i++)
			{
				QListWidgetItem* item = ui.chat_listView->item(i);
				ChatListItem* itemWidget = static_cast<ChatListItem*>(ui.chat_listView->itemWidget(item));

				if (itemWidget->GetIsOvered()) itemWidget->SetIsOvered(false);
			}
		}

		return QFramelessDialog::eventFilter(obj, event);
	}

	return QFramelessDialog::eventFilter(obj, event);
}

void StartUpWindow::Show()
{
	// 서버로 부터 받은 캐릭터 정보 처리
	QRect default_rect = AppUtils::GetScreenCenterGeometry(this, STARTUP_WIDGET_WIDTH, STARTUP_WIDGET_HEIGHT_MAX);
	QRect saved_rect = LocalSettings::GetWindowGeometry(metaObject()->className(), default_rect);
	if (!AppUtils::VerifyWindowRect(this, saved_rect))
	{
		saved_rect = default_rect;
		if (saved_rect.height() > STARTUP_WIDGET_HEIGHT_MAX) {
			saved_rect.setHeight(STARTUP_WIDGET_HEIGHT_MAX);
		}
		else if (saved_rect.height() < STARTUP_WIDGET_HEIGHT_MIN) {
			saved_rect.setHeight(STARTUP_WIDGET_HEIGHT_MIN);
		}
	}
	else
	{
		saved_rect.setWidth(STARTUP_WIDGET_WIDTH);
		saved_rect.setHeight(STARTUP_WIDGET_HEIGHT_MAX);
	}
	setGeometry(saved_rect);

	show();
}

GuildUserListItem* StartUpWindow::GetGuildUserListItem(int index)
{
	if(ui.shareWithGroup_widget->GetListWidget()->count() < index) return nullptr;

	QListWidgetItem* item = ui.shareWithGroup_widget->GetListWidget()->item(index);
	GuildUserListItem* itemWidget = static_cast<GuildUserListItem*>(ui.shareWithGroup_widget->GetListWidget()->itemWidget(item));
	return itemWidget;
}

void StartUpWindow::ClearGuildUserInfo()
{
	ui.shareWithGroup_widget->clearUser();
}

void StartUpWindow::OnCompleteChangeCharacter()
{
	// Set Webview Character
	SetWebviewCharacterInfo();

	// Hide circle indicator
	HideLoading();
}

void StartUpWindow::OnCreateChatting(bool success)
{
	if (!success)
	{
		LOG(ERROR) << format_string("[StartUpWindow::OnCreateChatting] failed");
		DisableAllButtons(false);
		AppUtils::DispatchToMainThread([]() {
			VideoChatMessageBox::Instance()->information(AppString::GetString("error_message/error_lime_create_chatting"));
		});
		return;
	}

	ChattingInfo m_chattingInfo = LimeCommunicator::Instance()->GetCreateChatInfo();
	std::string groupUserId = m_chattingInfo.groupUserInfo.groupUserId;
	std::string channelId = m_chattingInfo.channelId;
	StartStreaming(groupUserId, channelId);
}

void StartUpWindow::OnGetGuildUserList(bool success)
{
	if (!success)
	{
		LOG(ERROR) << format_string("[LimeCommunicator::OnGetGuildUserList] failed");
		AppUtils::DispatchToMainThread([]() {
			if (VideoChatMessageBox::Instance()->selection(AppString::GetString("error_message/error_lime_get_guild_user_list"), AppString::GetString("Common/retry"), AppString::GetString("Common/close")))
			{
				MainWindow::Get()->GetStartupWindow()->ReloadGuildUser();
			}
			else
			{
				MainWindow::Get()->AppClose();
			}
		});
		return;
	}

	GuildUserInfoList& _gulist = LimeCommunicator::Instance()->GetGuildUserList();
	for (int i = 0; i < _gulist.guildUserInfoList.size(); i++)
	{
		QListWidgetItem *item = new QListWidgetItem(ui.shareWithGroup_widget->GetListWidget());
		ui.shareWithGroup_widget->GetListWidget()->addItem(item);
		GuildUserListItem* theItem = new GuildUserListItem(this);
		item->setSizeHint(QSize(CHATLIST_ITEM_WIDTH, CHATLIST_ITEM_HEIGHT));
		ui.shareWithGroup_widget->GetListWidget()->SetItemWidget(item, theItem);
		theItem->SetIndex(i);
		theItem->SetGuildUserInfo(_gulist.guildUserInfoList[i]);
		ui.shareWithGroup_widget->UpdateProfileImage(i);
	}

	QString guildName{ LimeCommunicator::Instance()->GetGuildNamefromIndex(GetCharacterIndex()).c_str() };
	QString guildLabel{ App()->GetGuildLabel() };
	if (ui.shareWithGroup_widget->GetUserCount() <= 0)
	{
		ShowSharedWithGroupWidget(false);
		if (guildName != "")
		{
			ui.shareWithGroup_widget->HideLabelWidget(true);
			ui.shareWithGroup_widget->SetPrintText(ePrintText::emptyUser);
		}
	}
}

void StartUpWindow::OnGetShareTargetList(bool success)
{
	if (!success)
	{
		LOG(ERROR) << format_string("[LimeCommunicator::OnGetShareTargetList] failed");
		AppUtils::DispatchToMainThread([]() {
			if (VideoChatMessageBox::Instance()->selection(AppString::GetString("error_message/error_lime_get_share_target_list"), AppString::GetString("Common/retry"), AppString::GetString("Common/close")))
			{
				MainWindow::Get()->GetStartupWindow()->ReloadChatList();
			}
			else
			{
				MainWindow::Get()->AppClose();
			}
		});
		return;
	}

	if (success)
	{
		vector<GroupInfo> group_info = LimeCommunicator::Instance()->GetGroupInfo();
		ui.chat_listView->clear();

		int indexPlus = SortChatListItem(group_info);

		int i = 0;
		for (; i < group_info.size(); i++) {
			AddChatListItem(&group_info[i], i + indexPlus);
		}

		AddChatListItem(nullptr, i + indexPlus);
	}
	else
	{
		ui.chat_listView->clear();
		AddChatListItem(nullptr, 0);
		ui.startstream_btn->setDisabled(true);
	}

	if (ui.chat_listView->count() <= 0) {
		// 방송가능한 채팅방이 없는 경우
		ui.startstream_btn->setDisabled(true);
	}
	else {
		if (ui.shareWithGroup_widget->isHidden() == false)
		{
			ui.startstream_btn->setDisabled(true);
		}
		else
		{
			ui.startstream_btn->setDisabled(false);
		}
		ResetChatListItemSelect();
	}

	ResetHeight();

	OnCompleteChangeCharacter();
}

void StartUpWindow::OnLanguageChanged()
{
	MainWindow::Get()->GetStartupWindow()->SetLocalSavedMaskList();

	if (m_webViewWidget->IsShow())
	{
		QString theme = WebConfig::GetThemeString(App()->GetTheme());
		QString url = WebConfig::GetCreateChattingUrl(App()->GetServiceNetwork(), App()->GetToken().c_str(), theme);
		WebviewControl::Instance()->SetWebviewWidgetUrl(url, true);
	}
	else
	{
		WebviewControl::Instance()->SetWebviewWidgetUrl("", false);
	}
}

void StartUpWindow::ResetChatListItemSelect()
{
	if (m_newChatChannelId.isEmpty())
	{
		// Select arg ChatList
		ChatListItemRowChanged(0);
	}
	else
	{
		// Select New ChatList
		SetSelectNewChatListItem();
	}
}

bool StartUpWindow::ReloadGuildUser()
{
	ClearGuildUserInfo();

	GameUserInfo gameUserInfo;
	int characterIndex = LimeCommunicator::Instance()->GetCharacterIndex(App()->GetCharacterName().c_str());
	if (!LimeCommunicator::Instance()->GetGameUserInfo(characterIndex, gameUserInfo))
	{
		eloge("[StartUpWindow::ReloadGuildUser] failed GetGameUserInfo");
		return false;
	}

	if (gameUserInfo.guildName.empty())
	{
		elogi("[StartUpWindow::ReloadGuildUser] gameUserId(%s) Did not join a guild", gameUserInfo.gameUserId.c_str());
		return false;
	}
	
	return MainWindow::Get()->RequestGuildUserList();
}

void StartUpWindow::SelectAllGuildUser(bool bselectAll)
{
	for (int i = 0; i < ui.shareWithGroup_widget->GetListWidget()->count(); i++)
	{
		QListWidgetItem* item = ui.shareWithGroup_widget->GetListWidget()->item(i);
		GuildUserListItem* itemWidget = static_cast<GuildUserListItem*>(ui.shareWithGroup_widget->GetListWidget()->itemWidget(item));
		itemWidget->SetSelected(bselectAll);
	}
}

void StartUpWindow::SelectedCharacterChange(int index)
{
	// change selected icon ui
	for (int i = 0; i < m_characterSelectWidget->GetListWidget()->count(); i++)
	{
		QListWidgetItem* item = m_characterSelectWidget->GetListWidget()->item(i);
		CharacterListItem* itemWidget = static_cast<CharacterListItem*>(m_characterSelectWidget->GetListWidget()->itemWidget(item));

		bool bSelect = false;

		if (itemWidget->GetIndex() == index) {
			bSelect = true;
		}

		itemWidget->SetSelected(bSelect);
		itemWidget->update();
	}

	ui.characterSelect_btn->SetCharacterChagned();
}

void StartUpWindow::OnChangeCharactor(int index)
{
	ShowLoading();
	SelectedCharacterChange(index);
	SetCharacterIndex(index);
	ConnectPubServer();
}

void StartUpWindow::ProcessChangeCharacter(int index)
{
	QString guildName{ LimeCommunicator::Instance()->GetGuildNamefromIndex(index).c_str() };

	if (guildName == "")
	{
		if (ui.shareWithGroup_widget->isHidden() == false)
		{
			ui.shareWithGroup_widget->hide();
			ShowSharedWithGroupWidget(false);
		}
		ui.shareWithGroup_widget->HideLabelWidget(true);
		ui.shareWithGroup_widget->SetPrintText(ePrintText::emptyGuild);
	}
	else
	{
		ui.shareWithGroup_widget->SetGuildName(guildName);
		ui.shareWithGroup_widget->HideLabelWidget(false);
		ui.shareWithGroup_widget->SetPrintText(ePrintText::none);
	}

	// Reload Guild user
	ReloadGuildUser();

	// Reload Chat List
	ReloadChatList();
}

void StartUpWindow::ReloadChatList()
{
	int index = LimeCommunicator::Instance()->GetCharacterIndex(App()->GetCharacterName().c_str());
	vector<GroupInfo> group_info;
	if (LimeCommunicator::Instance()->GetGameUserInfo(index, m_currentSelectedUserInfo) == false) {
		// 캐릭터 정보를 받아오지 못한 경우 
		ui.chat_listView->clear();
		ui.startstream_btn->setDisabled(true);
		AddChatListItem(nullptr, 0);		// Add New ChatList
		VideoChatMessageBox::Instance()->information(AppString::GetString("error_message/error_lime_character_info"));
	}
	else {
		ui.startstream_btn->setDisabled(false);
	}

	MainWindow::Get()->RequestShareTargetList();
}

int StartUpWindow::SortChatListItem(vector<GroupInfo>& group_info)
{
	bool alaramMessageBox = false;
	int guildCnt = 0;
	vector<GroupInfo> tempGroupInfo;

	for (int j = 0; j < group_info.size(); j++)
	{
		QString group_type = QString::fromUtf8(group_info[j].groupType.c_str());
		QString game_channel_type = QString::fromUtf8(group_info[j].gameChannelType.c_str());
		bool checkWrongValueReceived = false;

		if (group_type != "GROUP" && group_type != "CHATTING") checkWrongValueReceived = true;
		if (game_channel_type != "GUILD" && game_channel_type != "ONE_ON_ONE" && game_channel_type != "") checkWrongValueReceived = true;
		if (group_info[j].shareChannelList.size() <= 1) checkWrongValueReceived = true;

		if (checkWrongValueReceived)
		{
			LOG(ERROR) << format_string("[StartUpWindow::SortChatListItem] [error] (gameChannelType : %s, groupId : %s, groupType : %s, name : %s, group_info[j].shareChannelList.size() : %d)",
				group_info[j].gameChannelType.c_str(),
				group_info[j].groupId.c_str(),
				group_info[j].groupType.c_str(),
				group_info[j].name.c_str(),
				group_info[j].shareChannelList.size());

			group_info.erase(group_info.begin() + j--);

			if (!alaramMessageBox) {
				VideoChatMessageBox::Instance()->information(AppString::GetString("error_message/error_lime_get_share_target_wrong"));
				alaramMessageBox = true;
			}

			continue;
		}

		// check is selected chatRoom
		if (App()->GetGroupId() == group_info[j].groupId) {
			AddChatListItem(&group_info[j], guildCnt);
			group_info.erase(group_info.begin() + j--);
			guildCnt++;
		}
		else
		{
			// check is guild chatRoom
			if (game_channel_type == "GUILD") {
				tempGroupInfo.push_back(group_info[j]);
				group_info.erase(group_info.begin() + j--);
			}
		}
	}

	if (tempGroupInfo.size())
	{
		for (int i = 0; i < tempGroupInfo.size(); i++)
		{
			AddChatListItem(&tempGroupInfo[i], guildCnt);
			guildCnt++;
		}
	}

	return guildCnt;
}

void StartUpWindow::ResetHeight()
{
	QRect rect = this->rect();
	int listCnt;
	int newh;

	if (ui.chat_listView->isHidden())
	{
		// show Guild Info
		listCnt = ui.shareWithGroup_widget->GetListWidget()->count();
		newh = listCnt * STARTUP_LIST_ITEM_HEIGHT + STARTUP_LIST_TOP_BOTTOM_MARGIN;
		newh += STARTUP_LIST_GUILD_TOP_MARGIN;
	}
	else
	{
		// show ChatList Info
		listCnt = ui.chat_listView->count();
		newh = listCnt * STARTUP_LIST_ITEM_HEIGHT + STARTUP_LIST_TOP_BOTTOM_MARGIN;
	}

	if (newh > STARTUP_WIDGET_HEIGHT_MAX)
	{
		newh = STARTUP_WIDGET_HEIGHT_MAX;
	}
	else if (newh < STARTUP_WIDGET_HEIGHT_MIN)
	{
		newh = STARTUP_WIDGET_HEIGHT_MIN;
	}

	setFixedHeight(newh);

	UpdateDimmingRect();
}

QPixmap* StartUpWindow::GetUserImageFromIndex(int index)
{
	if (index < 0) return nullptr;
	QListWidgetItem* item = m_characterSelectWidget->GetListWidget()->item(index);
	return !item ? nullptr : static_cast<CharacterListItem*>(m_characterSelectWidget->GetListWidget()->itemWidget(item))->GetCharacterImage();
}

void StartUpWindow::on_characterSelect_btn_clicked()
{
	if (m_micSettingWidget->GetHide() == false) m_micSettingWidget->hide();
	if (m_maskSettingWidget->GetHide() == false) m_maskSettingWidget->hide();
	if (m_webViewWidget->GetHide() == false) m_webViewWidget->hide();
	HideChatListItemPopUp();

	if (m_characterSelectWidget)
	{
		if (m_characterSelectWidget->GetHide())
		{
			m_characterSelectWidget->show();
		}
		else
		{
			m_characterSelectWidget->hide();
		}
	}
}

void StartUpWindow::on_micsetting_btn_clicked()
{
	HideChatListItemPopUp();

	if (m_micSettingWidget)
	{
		if (m_micSettingWidget->GetHide())
		{
			m_micSettingWidget->show();
		}
		else
		{
			m_micSettingWidget->hide();
		}
	}
}

void StartUpWindow::on_mask_btn_clicked()
{
	HideChatListItemPopUp();

	if (m_maskSettingWidget)
	{
		if (m_maskSettingWidget->GetHide())
		{
			m_maskSettingWidget->show();
		}
		else
		{
			m_maskSettingWidget->hide();
		}
	}
}

bool StartUpWindow::GetCharacterSelectWidgetIsHidden()
{
	if (m_characterSelectWidget)
	{
		if (m_characterSelectWidget->GetHide())
		{
			return true;
		}
	}
	return false;
}

void StartUpWindow::SetGameUserInfoList()
{
	for (int index = 0; index < LimeCommunicator::Instance()->GetCurrentCharacterCount(); index++)
	{
		if (LimeCommunicator::Instance()->GetCanStreamfromIndex(index))
		{
			UpdateProfileImage(index);
			AddCharacterListItem(index);
		}
	}
}

void StartUpWindow::AddCharacterListItem(int index)
{
	if (m_characterSelectWidget)
	{
		QListWidgetItem *item = new QListWidgetItem(m_characterSelectWidget->GetListWidget());
		m_characterSelectWidget->GetListWidget()->addItem(item);
		CharacterListItem* theItem = new CharacterListItem(this);
		item->setSizeHint(QSize(CHATLIST_ITEM_WIDTH, CHATLIST_ITEM_HEIGHT));
		m_characterSelectWidget->GetListWidget()->SetItemWidget(item, theItem);
		theItem->SetIndex(index);
	}
}

void StartUpWindow::AddMicListItem(int index, AudioDeviceInfo* info)
{
	if (m_micSettingWidget)
	{
		QListWidgetItem *item = new QListWidgetItem(m_micSettingWidget->GetListWidget());
		m_micSettingWidget->GetListWidget()->addItem(item);
		MicSettingListItem* theItem = new MicSettingListItem(this);
		item->setSizeHint(QSize(MICLIST_ITEM_WIDTH, MICLIST_ITEM_HEIGHT));
		m_micSettingWidget->GetListWidget()->setItemWidget(item, theItem);
		theItem->SetIndex(index);
		theItem->SetMicInfo(info);
	}
}

void StartUpWindow::UpdateProfileImage(int index)
{
	QNetworkAccessManager *manager = new QNetworkAccessManager(this);
	connect(manager, &QNetworkAccessManager::finished, this, &StartUpWindow::ProfileImageDownloadFinished);
	connect(manager, &QNetworkAccessManager::finished, manager, &QNetworkAccessManager::deleteLater);

	const QUrl url = QUrl(LimeCommunicator::Instance()->GetCharacterImageUrlfromIndex(index).c_str());

	QNetworkRequest request(url);
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

	QNetworkReply* reply = manager->get(request);
	reply->setObjectName(QString::number(index));
}

void StartUpWindow::ProfileImageDownloadFinished(QNetworkReply *reply)
{
	int profileImageSize = CHARACTER_IMAGE_SIZE;
	QPixmap pm;
	if (!pm.loadFromData(reply->readAll())) {
		pm.load(":/StartStreamDlg/image/ic_default_character.png");
	}
	pm = pm.scaled(profileImageSize, profileImageSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

	QImage dst(profileImageSize, profileImageSize, QImage::Format_ARGB32);
	dst.fill(QColor(0, 0, 0, 0));

	QPainter p(&dst);
	QPainterPath path;
	path.addRoundedRect(0, 0, profileImageSize, profileImageSize, profileImageSize / 2, profileImageSize / 2);

	p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, true);

	QPen pen(CHARACTER_IMAGE_BORDER_COLOR, CHARACTER_IMAGE_BORDER);
	p.setPen(pen);

	p.fillPath(path, QBrush(Qt::white));

	p.setCompositionMode(QPainter::CompositionMode_SourceAtop);

	p.drawPixmap(dst.rect(), pm);
	p.drawPath(path);

	int index = reply->objectName().toInt();
	QListWidgetItem* item = m_characterSelectWidget->GetListWidget()->item(index);

	if (index == LimeCommunicator::Instance()->GetCurrentCharacterIndex())
	{
		ui.characterSelect_btn->update();
	}

	return static_cast<CharacterListItem*>(m_characterSelectWidget->GetListWidget()->itemWidget(item))->SetCharacterImage(QPixmap::fromImage(dst));
}

void StartUpWindow::SetShowWebviewWidget(bool show)
{
	if (show)
		m_webViewWidget->show();
	else
		m_webViewWidget->hide();
}

void StartUpWindow::SetWebviewUrl(QString url, bool forced)
{
	m_webViewWidget->SetUrl(url, forced);
}

QString StartUpWindow::GetWebviewCharacterId()
{
	return m_currentSelectedUserInfo.gameUserId.c_str();
}

QString StartUpWindow::GetWebviewCharacterInfo()
{
	QString gameUserId = m_currentSelectedUserInfo.gameUserId.c_str();
	QString gameCode = App()->GetCharacterCode().c_str();
	return WebviewControl::Instance()->MakeCharacterInfo(gameUserId, gameCode);
}

void StartUpWindow::SetWebviewCharacterInfo()
{
	QString gameUserId = m_currentSelectedUserInfo.gameUserId.c_str();
	QString gameCode = App()->GetCharacterCode().c_str();
	QString characterInfo = WebviewControl::Instance()->MakeCharacterInfo(gameUserId, gameCode);

	WebCommunicator::Instance()->SetCharactorId(gameUserId);
	WebCommunicator::Instance()->SetCharactorInfo(characterInfo);

	if (m_webViewWidget->IsShow() && m_webViewWidget->GetWebview()) 
	{
		WebCommunicator::Instance()->SetWebView(m_webViewWidget->GetWebview());
		WebviewControl::Instance()->SetCharacterId(m_webViewWidget->GetWebview(), gameUserId);
		WebviewControl::Instance()->SetCharacterInfo(m_webViewWidget->GetWebview(), characterInfo);
	}
}

void StartUpWindow::SetShowLoading(bool show)
{
	if (show)
		ShowLoading();
	else
		HideLoading();
}

void StartUpWindow::ShowLoading(QWidget* child)
{
	m_dimmingCnt++;

	// show dimming
	ui.characterSelect_btn->setDisabled(true);

	// make dimming painter
	if (m_dimmingPainter.isNull())
	{
		m_dimmingPainter.reset(new dimmingPainter(this));
	}

	if (child)
	{
		m_dimmingPainter->SetChild(child);
		child->raise();
	}
	else
	{
		m_dimmingPainter->ShowIndicator();
	}

	UpdateDimmingRect();
	if(m_dimmingPainter->isHidden())
		m_dimmingPainter->show();
}

void StartUpWindow::ShowDimmingIndicator()
{
	if (m_dimmingPainter.isNull() == false)
	{
		m_dimmingPainter->ShowIndicator();
	}
}

bool StartUpWindow::IsDimmingShow()
{
	if (m_dimmingPainter.isNull())
	{
		return false;
	}
	else
	{
		return !(m_dimmingPainter->isHidden());
	}
}

void StartUpWindow::UpdateDimmingRect()
{
	if (m_dimmingPainter.isNull() == false)
	{
		QRect r = rect();
		r.setY(r.y() + getTitlebarHeight());
		m_dimmingPainter->SetPaintRect(r);
	}
}

void StartUpWindow::HideLoading()
{
	m_dimmingCnt--;

	m_dimmingPainter->SetChild(Q_NULLPTR);

	// hide dimming
	if (m_dimmingPainter.isNull() == false && m_dimmingCnt <= 0)
	{
		ui.characterSelect_btn->setDisabled(false);
		m_dimmingPainter->Hide();
		m_dimmingCnt = 0;
	}
}

void StartUpWindow::CreatedChattingChannel(const QString& channelid)
{
	ShowLoading();

	ReloadChatList();

	m_newChatChannelId = channelid;
}

void StartUpWindow::SetSelectNewChatListItem()
{
	for (int i = 0; i < ui.chat_listView->count(); i++)
	{
		QListWidgetItem* item = ui.chat_listView->item(i);
		ChatListItem* itemWidget = static_cast<ChatListItem*>(ui.chat_listView->itemWidget(item));

		if (itemWidget->CheckHaveChannelID(m_newChatChannelId))
		{
			itemWidget->SetSelected(true);
			itemWidget->update();
			ui.chat_listView->scrollToItem(item, QAbstractItemView::PositionAtTop);
		}
		else
		{
			if (itemWidget->GetSelected())
			{
				itemWidget->SetSelected(false);
				itemWidget->update();
			}
		}
	}

	m_newChatChannelId = "";
	HideLoading();
}

void StartUpWindow::ChatListItemActive(QListWidgetItem *item)
{
	if (m_startingStream) return;

	if (static_cast<ChatListItem*>(ui.chat_listView->itemWidget(item))->IsMakeNewChat())
	{
		QString theme = WebConfig::GetThemeString(App()->GetTheme());
		QString url = WebConfig::GetCreateChattingUrl(App()->GetServiceNetwork(), App()->GetToken().c_str(), theme);
		WebviewControl::Instance()->SetWebviewWidgetUrl(url, false);
		WebviewControl::Instance()->SetShowWebviewWidget(true);
		SetWebviewCharacterInfo();
	}
}

void StartUpWindow::InitUI()
{
	setWindowIcon(QIcon(QString::fromStdString(AppConfig::purplelive_icon_path)));

	ui.chat_listView->connect(ui.chat_listView, QOverload<int>::of(&QListWidget::currentRowChanged), this, QOverload<int>::of(&StartUpWindow::ChatListItemRowChanged));
	ui.chat_listView->connect(ui.chat_listView, QOverload<QListWidgetItem *>::of(&QListWidget::itemDoubleClicked), this, QOverload<QListWidgetItem *>::of(&StartUpWindow::ChatListItemActive));
	ui.chat_listView->connect(ui.chat_listView, QOverload<QListWidgetItem *>::of(&QListWidget::itemPressed), this, QOverload<QListWidgetItem *>::of(&StartUpWindow::ChatListItemActive));
	
	ui.chat_listView->viewport()->installEventFilter(this);

	ui.chat_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	ui.chat_listView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

	ui.shareWithGroup_widget->hide();
	ui.chat_listView->show();

	ui.chat_listView->setFixedWidth(STARTUP_WIDGET_WIDTH - LISTWIDGET_X_MARGN);
	ui.shareWithGroup_widget->setFixedWidth(STARTUP_WIDGET_WIDTH - LISTWIDGET_X_MARGN);

	// defualt ui is Shared with Chat
	ShowSharedWithGroupWidget(false);

	ui.vertical_line->setFixedHeight(1);
}

void StartUpWindow::InitUIAfterCreateChild()
{
	QObject::connect(MainWindow::Get(), &MainWindow::UpdateChildLanguageString, this, &StartUpWindow::UpdateLanguageString);

	ui.characterSelect_btn->InitUIAfterCreation();
	ui.shareWithGroup_widget->InitUIAfterCreation();

	UpdateLanguageString();
}

int StartUpWindow::GetChatListSelectedRow()
{
	for (int i = 0; i < ui.chat_listView->count(); i++)
	{
		QListWidgetItem* item = ui.chat_listView->item(i);
		ChatListItem* itemWidget = static_cast<ChatListItem*>(ui.chat_listView->itemWidget(item));

		if (itemWidget->GetSelected())
		{
			return i;
		}
	}

	return -1;
}

void StartUpWindow::ChatListItemRowChanged(int row)
{
	if (row < 0) return;
	if (m_startingStream) return;

	for (int i = 0; i < ui.chat_listView->count(); i++)
	{
		QListWidgetItem* item = ui.chat_listView->item(i);
		ChatListItem* itemWidget = static_cast<ChatListItem*>(ui.chat_listView->itemWidget(item));

		if (i != row) {
			if (itemWidget->GetSelected() == false) continue;
			itemWidget->SetSelected(false);
		}
		else itemWidget->SetSelected(true);

		itemWidget->update();
	}

	if (row == ui.chat_listView->count() - 1)
	{
		ui.startstream_btn->setDisabled(true);
	}
	else
	{
		if(ui.shareWithGroup_widget->isVisible())
			ui.startstream_btn->setDisabled(true);
		else
			ui.startstream_btn->setDisabled(false);
	}
}

void StartUpWindow::ShowSharedWithGroupWidget(bool bshow)
{
	bool bSharedWidthGroup = ui.sharedWithGroup_btn->property("isChecked").toBool();
	bool bSharedWidthChat = ui.sharedWithChat_btn->property("isChecked").toBool();

	if (m_showDropdownShadowEffect.isNull())
	{
		m_showDropdownShadowEffect.reset(new QGraphicsDropShadowEffect());
		m_showDropdownShadowEffect->setBlurRadius(2);
		m_showDropdownShadowEffect->setColor(QColor(0, 0, 0, 255 * 0.08));
		m_showDropdownShadowEffect->setXOffset(0);
		m_showDropdownShadowEffect->setYOffset(2);
	}
	

	if (bshow)
	{
		// show Share With Group Widget

		// insert button shadow
		ui.sharedWithGroup_btn->setGraphicsEffect(m_showDropdownShadowEffect.data());
		delete ui.sharedWithChat_btn->graphicsEffect();
		ui.buttonSelectWidget->update();

		if (ui.shareWithGroup_widget->GetSelectedUserCount() == 0)
		{
			SetStreamBtnDisable(true);
		}
		else
		{
			SetStreamBtnDisable(false);
		}

		if (bSharedWidthGroup == true)
		{
			if (bSharedWidthChat == true) {
				ui.sharedWithChat_btn->setProperty("isChecked", false);
			}
			else {
				
			}
		}
		else {
			ui.sharedWithGroup_btn->setProperty("isChecked", true);
			ui.sharedWithChat_btn->setProperty("isChecked", false);
		}

		ui.chat_listView->hide();
		if(ui.shareWithGroup_widget->isHidden()) ui.shareWithGroup_widget->show();
		HideChatListItemPopUp();
	}
	else
	{
		// insert button shadow
		ui.sharedWithChat_btn->setGraphicsEffect(m_showDropdownShadowEffect.data());
		delete ui.sharedWithGroup_btn->graphicsEffect();
		ui.buttonSelectWidget->update();

		if (bSharedWidthChat == true)
		{
			if (bSharedWidthGroup == true) {
				ui.sharedWithGroup_btn->setProperty("isChecked", false);
			}
			else {
				
			}
		}
		else {
			ui.sharedWithChat_btn->setProperty("isChecked", true);
			ui.sharedWithGroup_btn->setProperty("isChecked", false);
		}

		ui.shareWithGroup_widget->hide();
		if (ui.chat_listView->isHidden()) ui.chat_listView->show();


		// show Share With Chat List
		ChatListItemRowChanged(GetChatListSelectedRow());
	}

	ui.sharedWithChat_btn->style()->unpolish(ui.sharedWithChat_btn);
	ui.sharedWithChat_btn->style()->polish(ui.sharedWithChat_btn);
	ui.sharedWithGroup_btn->style()->unpolish(ui.sharedWithGroup_btn);
	ui.sharedWithGroup_btn->style()->polish(ui.sharedWithGroup_btn);

	ResetHeight();
}

void StartUpWindow::on_sharedWithChat_btn_clicked()
{
	ShowSharedWithGroupWidget(false);
}

void StartUpWindow::on_sharedWithGroup_btn_clicked()
{
	ShowSharedWithGroupWidget(true);
}

void StartUpWindow::SetMicSelection(int index)
{
	if (m_micSettingWidget->GetListWidget()->count() < index) return;

	for (int i = 0; i < m_micSettingWidget->GetListWidget()->count(); i++)
	{
		bool bselect = false;
		if (i == index) {
			bselect = true;
			m_micSettingWidget->SetSelectedIndex(index);
			ui.micsetting_btn->SetInfoName(m_micSettingWidget->GetMicNameFromIndex(index));
		}

		QListWidgetItem* item = m_micSettingWidget->GetListWidget()->item(i);
		MicSettingListItem* itemWidget = static_cast<MicSettingListItem*>(m_micSettingWidget->GetListWidget()->itemWidget(item));
		itemWidget->SetSelected(bselect);
	}
}

bool StartUpWindow::GetMicIsDefault(int index)
{
	if (m_micSettingWidget->GetListWidget()->count() < index) return false;

	QListWidgetItem* item = m_micSettingWidget->GetListWidget()->item(index);
	MicSettingListItem* itemWidget = static_cast<MicSettingListItem*>(m_micSettingWidget->GetListWidget()->itemWidget(item));
	return itemWidget->GetDefault();
}

void StartUpWindow::SetLocalSavedMicList()
{
	float volume = LocalSettings::Instance()->GetMicVolume(AppConfig::default_volume);
	bool mute = LocalSettings::Instance()->GetMicMute(AppConfig::default_mute);

	MainWindow::Get()->SetMicVolume(volume);

	if (m_micSettingWidget)
	{
		m_micSettingWidget->SetVolume(volume);
		m_micSettingWidget->SetMute(mute);
	}

	QString savedMicDeviceID;
	savedMicDeviceID = LocalSettings::GetSelectedMicDeviceID(savedMicDeviceID);

	if (savedMicDeviceID == AppConfig::not_use_mic_device) {
		SetMicSelection(m_micSettingWidget->GetListWidget()->count() - 1);
		return;
	}

	int defaultMicIndex = 0;

	for (int i = 0; i < m_micSettingWidget->GetListWidget()->count(); i++)
	{
		if (GetMicDeviceID(i) == savedMicDeviceID)
		{
			defaultMicIndex = i;
			break;
		}

		if (GetMicIsDefault(i)) {
			defaultMicIndex = i;
		}
	}

	SetMicSelection(defaultMicIndex);
}

void StartUpWindow::SetMicList()
{
	MainWindow* pmainWindow = static_cast<MainWindow*>(App()->GetMainWindow());

	AudioDeviceInfo tempMic;
	vector<AudioDeviceInfo> tempMicList;

	tempMic.name = string(AppString::GetQString("Common/un_used").toLocal8Bit());
	pmainWindow->GetMicInfoList(tempMicList);
	tempMicList.push_back(tempMic);

	bool micListChanged = false;

	if (m_micSettingWidget->GetListWidget()->count() == tempMicList.size()) {
		for (int i = 0; i < tempMicList.size(); i++) 
		{
			QListWidgetItem* item = m_micSettingWidget->GetListWidget()->item(i);
			MicSettingListItem* itemWidget = static_cast<MicSettingListItem*>(m_micSettingWidget->GetListWidget()->itemWidget(item));

			if (itemWidget->GetMicName().toStdString() != tempMicList[i].name) {
				micListChanged = true;
				break;
			}
		}
	}
	else {
		micListChanged = true;
	}

	if (micListChanged) {
		if (tempMicList.size() > 0) {
			// find Default Mic
			int micIndex = 1;
			for (auto& micItem = tempMicList.begin(); micItem != tempMicList.end(); micItem++)
			{
				if (micItem->is_default)
				{
					micItem->name = AppUtils::format(StringUtils::GetStringFromUtf8(AppString::GetString("main_menu/main_menu_default_device_format")).c_str(), micItem->name.c_str());
					AddMicListItem(micIndex++, &(*micItem));
					tempMicList.erase(micItem);
					break;
				}
			}

			for (auto& micItem : tempMicList)
			{
				if (micItem.name == string(AppString::GetQString("Common/un_used").toLocal8Bit()))
					continue;

				AddMicListItem(micIndex++, &micItem);
			}
		}
		else {
			// empty mic
		}

		AddMicListItem(0);		// 사용안함
	}

	return;
}

QString StartUpWindow::GetCurrentSelectedGroupUserID()
{
	for (int i = 0; i < ui.chat_listView->count(); i++)
	{
		QListWidgetItem* item = ui.chat_listView->item(i);
		ChatListItem* itemWidget = static_cast<ChatListItem*>(ui.chat_listView->itemWidget(item));

		if (itemWidget->GetGroupUserID() != "")
		{
			return itemWidget->GetGroupUserID();
		}
	}

	return "";
}

QString StartUpWindow::GetCurrentSelectedChannelID()
{
	for (int i = 0; i < ui.chat_listView->count(); i++)
	{
		QListWidgetItem* item = ui.chat_listView->item(i);
		ChatListItem* itemWidget = static_cast<ChatListItem*>(ui.chat_listView->itemWidget(item));

		if (itemWidget->GetChannelID() != "")
		{
			return itemWidget->GetChannelID();
		}
	}

	return "";
}

GuildUserInfoList& StartUpWindow::GetSelectedGroupList()
{
	GuildUserInfoList& _gulist = LimeCommunicator::Instance()->GetGuildUserList();

	m_selectedGroupInfo.guildUserInfoList.clear();
	m_selectedGroupInfo.guildName = _gulist.guildName;
	m_selectedGroupInfo.gameCode = _gulist.gameCode;

	for (int i = 0; i < ui.shareWithGroup_widget->GetListWidget()->count(); i++)
	{
		QListWidgetItem* item = ui.shareWithGroup_widget->GetListWidget()->item(i);
		GuildUserListItem* itemWidget = static_cast<GuildUserListItem*>(ui.shareWithGroup_widget->GetListWidget()->itemWidget(item));
		if (itemWidget->GetSelected())
		{
			m_selectedGroupInfo.guildUserInfoList.push_back(itemWidget->GetGuildUserInfo());
		}
	}

	return m_selectedGroupInfo;
}

void StartUpWindow::DisableAllButtons(bool disable)
{
	SetShowLoading(disable);
	m_startingStream = disable;
	ui.characterSelect_btn->setDisabled(disable);
	ui.sharedWithChat_btn->setDisabled(disable);
	ui.sharedWithGroup_btn->setDisabled(disable);
	ui.micsetting_btn->setDisabled(disable);
	ui.mask_btn->setDisabled(disable);
	ui.startstream_btn->SetPrepareStream(disable);
}

void StartUpWindow::StartStreaming(std::string& groupUserId, std::string& channelId)
{
	if (groupUserId == "" || channelId == "")
	{
		DisableAllButtons(false);
		return;
	}

	MainWindow::Get()->SetTarget(App()->GetGameHwnd());

	std::string characterName = LimeCommunicator::Instance()->GetCharacterNamefromIndex(LimeCommunicator::Instance()->GetCurrentCharacterIndex());
	QString roomNameMultiByte = AppString::GetQString("StreamDlg/default_room_title_prefix") + QString::fromStdString(characterName) + AppString::GetQString("StreamDlg/default_room_title_suffix");
	std::string roomName = roomNameMultiByte.toUtf8();
	std::string roomId = "";

	auto OnChangeStreamingState = [this](eStreamingState streamingState) {
		LOG(INFO) << format_string("[StartUpWindow::OnChangeStreamingState] streamingState(%d)", streamingState);
		switch (streamingState)
		{
		case eStreamingState::eSS_None:
			break;
		case eStreamingState::eSS_Starting:
			break;
		case eStreamingState::eSS_Streaming:
			SetActive(false);
			MainWindow::Get()->SetActive(true);
			MainWindow::Get()->SetChangeStreamingStateCallback(nullptr);
			break;
		case eStreamingState::eSS_Stopping:
			break;
		case eStreamingState::eSS_Stopped:
			break;
		case eStreamingState::eSS_Error:
			DisableAllButtons(false);
			MainWindow::Get()->StopRender();
			MainWindow::Get()->SetChangeStreamingStateCallback(nullptr);
			break;
		}
	};

	bool mic_selected = m_micSettingWidget->GetItemCount() != m_micSettingWidget->GetSelectedIndex() + 1;
	MainWindow::Get()->SetEnableInputAudioCapture(mic_selected);
	MainWindow::Get()->SetMicSelect(m_micSettingWidget->GetSelectedIndex());

	MainWindow::Get()->SetLocalSavedMicSetting();
	MainWindow::Get()->SetTitle(QString::fromStdString(characterName));

	LimeCommunicator::Instance()->SetStreamingState(eStreamingState::eSS_None);
	MainWindow::Get()->SetChangeStreamingStateCallback(OnChangeStreamingState);
	MainWindow::Get()->StartRender(App()->GetForceExcute(), roomId.c_str(), roomName.c_str(), groupUserId.c_str(), channelId.c_str());
}

void StartUpWindow::on_startstream_btn_clicked()
{
	DisableAllButtons(true);

	if (false == CheckServiceInspection())
		return;

	if (ui.chat_listView->isHidden())
	{
		// Make New GuildChat
		if (ui.shareWithGroup_widget->GetSelectedUserCount() == 0)
		{
			DisableAllButtons(false);
			return;
		}
		GuildUserInfoList r = StartUpWindow::GetSelectedGroupList();
		MainWindow::Get()->RequestCreateChatting(GetSelectedGroupList());
	}
	else
	{
		std::string groupUserId = GetCurrentSelectedGroupUserID().toUtf8();
		std::string channelId = GetCurrentSelectedChannelID().toUtf8();
		StartStreaming(groupUserId, channelId);
	}
}

void StartUpWindow::on_close_btn_clicked()
{
	MainWindow::Get()->AppClose();
}

void StartUpWindow::closeEvent(QCloseEvent * event)
{
	if (this->isHidden() == false) {
		if(m_micSettingWidget->isHidden() == false) m_micSettingWidget->hide();
		SetMicList();
		if (m_micSettingWidget)
		{
			LocalSettings::SetMicMute(m_micSettingWidget->GetMuted());
			LocalSettings::SetMicVolume(m_micSettingWidget->GetVolume());
			int micIndex = m_micSettingWidget->GetSelectedIndex();
			LocalSettings::SetSelectedMicDeviceID(GetMicDeviceID(micIndex));
		}
	}
}

void StartUpWindow::on_min_btn_clicked()
{
	setWindowState(Qt::WindowMinimized);
}

void StartUpWindow::AddChatListItem(GroupInfo* gi, int index)
{
	QListWidgetItem *item = new QListWidgetItem(ui.chat_listView);
	ui.chat_listView->addItem(item);
	bool isMakeNewChat;

	if (gi)
	{
		isMakeNewChat = false;
	}
	else
	{
		isMakeNewChat = true;
	}

	ChatListItem *theItem = new ChatListItem(this, index, isMakeNewChat);

	theItem->SetGroupInfo(gi);
	item->setSizeHint(QSize(CHATLIST_ITEM_WIDTH, CHATLIST_ITEM_HEIGHT));
	ui.chat_listView->SetItemWidget(item, theItem);
}

int StartUpWindow::GetUserCharacCount()
{
	return LimeCommunicator::Instance()->GetCurrentCharacterCount();
}

QString StartUpWindow::GetMicDeviceID(int index)
{
	if (m_micSettingWidget->GetListWidget()->count() - 1 <= index) return AppConfig::not_use_mic_device;

	QListWidgetItem* item = m_micSettingWidget->GetListWidget()->item(index);
	MicSettingListItem* itemWidget = static_cast<MicSettingListItem*>(m_micSettingWidget->GetListWidget()->itemWidget(item));
	return itemWidget->GetMicID();
}

int StartUpWindow::GetCharacterListHeight()
{
	if (m_characterSelectWidget)
	{
		return m_characterSelectWidget->GetListWidgetHeight();
	}
	return 0;
}

void StartUpWindow::UpdateLanguageString()
{
	// set text
	this->setWindowTitle(QString::fromStdString(AppString::GetQString("Common/service_name").toStdString()));
	ui.startstream_btn->setText("");
	ui.startstream_btn->UpdateLanguageString();
	ui.micsetting_btn->SetLabelText(AppString::GetQString("StreamDlg/mic_setting_btn"));
	ui.mask_btn->SetLabelText(AppString::GetQString("StreamDlg/mask_btn"));
	ui.sharedWithChat_btn->setText(AppString::GetQString("StreamDlg/share_with_chat_checkbox"));

	QString guildLabel{ App()->GetGuildLabel() };
	QString btnText = QString::fromStdString(AppUtils::format(AppString::GetString("StreamDlg/share_with_group_checkbox").c_str(), guildLabel.toStdString().c_str()));
	ui.sharedWithGroup_btn->setText(btnText);
	
	update();
}
