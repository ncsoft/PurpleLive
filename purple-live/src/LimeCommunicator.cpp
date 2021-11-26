#include "LimeCommunicator.h"
#include "LimeLogin.hpp"
#include "MAPManager.h"
#include "VideoChatApp.h"
#include <QMessageBox>
#include "AppUtils.h"
#include "easylogging++.h"
#include "VideoChatMessageBox.h"
#include "AppString.h"

LimeCommunicator::LimeCommunicator()
{

}

LimeCommunicator::~LimeCommunicator()
{
	StopRetryConnection();
	elogd("[LimeCommunicator::~LimeCommunicator] Singleton");
}

void LimeCommunicator::SetForceExcute(bool enable)
{
	
}

void LimeCommunicator::SetRoomID(const char* id)
{
	m_roomId = StringUtils::ValueOrEmpty(id);
}

void LimeCommunicator::SetRoomName(const char* name)
{
	m_roomName = StringUtils::ValueOrEmpty(name);
}

void LimeCommunicator::SetShareTarget(const char* groupUserId, const char* channelId)
{
	m_groupUserId = StringUtils::ValueOrEmpty(groupUserId);
	m_channelId = StringUtils::ValueOrEmpty(channelId);
}

void LimeCommunicator::SetFirstShareTarget()
{
	if (GetShareTargetList(GetFirstGameUserID().c_str(), App()->GetToken().c_str(), App()->GetDeviceId().c_str(), App()->GetServiceNetwork(), App()->GetAppVersion().c_str()))
	{
		if (m_groupInfoList.size() > 0)
		{
			if(m_groupInfoList[0].shareChannelList.size() > 0)
			{
				m_groupUserId = m_groupInfoList[0].shareChannelList[0].groupUserId;
				m_channelId = m_groupInfoList[0].shareChannelList[0].channelId;
			}
		}
	}
}

bool LimeCommunicator::GetGameUserInfo(int index, GameUserInfo& game_user_info)
{
	if (m_vtrCharacters.size() <= index)
		return false;

	game_user_info = m_vtrCharacters[index];
	return true;
}

int LimeCommunicator::GetCharacterIndex(const char* characterName)
{
	for (int i = 0; i < m_vtrCharacters.size(); i++)
	{
		if (characterName == StringUtils::GetStringFromUtf8(m_vtrCharacters[i].characterName))
		{
			return i;
		}
	}
	return -1;
}

bool LimeCommunicator::GetCanStreamfromIndex(int index)
{
	if (m_vtrCharacters.size() <= index)
		return false;

	return m_vtrCharacters[index].canStreaming;
}

string LimeCommunicator::GetGuildNamefromIndex(int index)
{
	if (m_vtrCharacters.size() <= index)
		return "";

	return m_vtrCharacters[index].guildName;
}

string LimeCommunicator::GetGamecodefromIndex(int index)
{
	if (m_vtrCharacters.size() <= index)
		return "";

	return m_vtrCharacters[index].gameCode;
}

string LimeCommunicator::GetCharacterNamefromIndex(int index)
{
	if (m_vtrCharacters.size() <= index)
		return "";

	return m_vtrCharacters[index].characterName;
}

std::string	LimeCommunicator::GetCharacterServerNamefromIndex(int index)
{
	if (m_vtrCharacters.size() <= index)
		return "";

	return m_vtrCharacters[index].serverName;
}

std::string	LimeCommunicator::GetCharacterImageUrlfromIndex(int index)
{
	if (m_vtrCharacters.size() <= index)
		return "";

	return m_vtrCharacters[index].profileUrl;
}

int	LimeCommunicator::GetCurrentCharacterCount()
{
	return m_vtrCharacters.size();
}

std::string LimeCommunicator::GetFirstGameUserID()
{
	int index = 0;
	if (m_vtrCharacters.size() <= index) {
		return "";
	}
	return m_vtrCharacters[index].gameUserId;
}

std::string LimeCommunicator::GetGameUserID(const char* characterName)
{
	if (StringUtils::ValueOrEmpty(characterName).empty())
		return GetFirstGameUserID();

	for (int i=0; i<m_vtrCharacters.size(); i++)
	{
		if (characterName==StringUtils::GetStringFromUtf8(m_vtrCharacters[i].characterName))
		{
			return m_vtrCharacters[i].gameUserId;
		}
	}
	return std::string{};
}

std::string LimeCommunicator::GetUserImageFromCharacterName(const char* characterName)
{
	for (int i = 0; i < m_vtrCharacters.size(); i++)
	{
		if (QString::fromUtf8(m_vtrCharacters[i].characterName.c_str()) == QString::fromLocal8Bit(characterName))
		{
			return m_vtrCharacters[i].profileUrl;
		}
	}
	return std::string{};
}

int	LimeCommunicator::GetCurrentCharacterIndex()
{
	return m_current_charactor_index;
}

void LimeCommunicator::SetCurrentCharacterIndex(int index)
{
	m_current_charactor_index = index;
}

std::string LimeCommunicator::GetAuthKey()
{
	return m_authKey;
}

std::string	LimeCommunicator::GetCreateChattingParams(const char* gameUserID, const GuildUserInfoList& info)
{
	json_t* root = json_object();
	json_object_set(root, "gameUserId", json_string(gameUserID));
	json_object_set(root, "groupUserId", json_null());
	json_object_set_new(root, "inviteUserInfoList", json_array());
	json_t* inviteUserInfoList = json_object_get(root, "inviteUserInfoList");

	for (int i=0; i<info.guildUserInfoList.size(); i++)
	{
		json_t* item = json_object();
		json_object_set(item, "gameUserId", json_string(info.guildUserInfoList[i].gameUserId.c_str()));
		json_object_set(item, "gameCode", json_null());
		json_object_set(item, "location", json_null());
		json_object_set(item, "serverId", json_null());
		json_object_set(item, "gameCode", json_null());
		json_object_set(item, "characterId", json_null());
		json_array_append(inviteUserInfoList, item);
	}

	std::string params = json_dumps(root, 0);
	elogd("[LimeCommunicator::GetCreateChattingParams] message(%s)", params.c_str());
	return params;
}

std::string	LimeCommunicator::GetInviteLinkUrl()
{
	return m_roomInfo.inviteLinkUrl;
}

bool LimeCommunicator::GetCharactorList(const char* gameCode, const char* token, eServiceNetwork sn, const char* appVersion)
{
	m_vtrCharacters.clear();
	eGetCharacterErrorType e = ::GetCharacterList(gameCode, token, m_vtrCharacters, sn, appVersion, App()->GetLocalLanguage().toStdString().c_str());
	switch(e)
	{
		case eGetCharacterErrorType::eNoError:
			LOG(DEBUG) << format_string("[LimeCommunicator::GetCharactorList] success characters(%d)", m_vtrCharacters.size());
			return true;
		case eGetCharacterErrorType::eNoStreamableCharacter:
			LOG(ERROR) << format_string("[LimeCommunicator::GetCharactorList] error(eNoStreamableCharacter)");
			MainWindow::Get()->AppClose(AppString::GetString("error_message/error_lime_no_streamable_character").c_str());
			break;
		case eGetCharacterErrorType::eFailFindCharacter:
		default:
			LOG(ERROR) << format_string("[LimeCommunicator::GetCharactorList] error(%d)", e);
			if (VideoChatMessageBox::Instance()->selection(AppString::GetString("error_message/error_lime_get_character_list"), AppString::GetString("Common/retry"), AppString::GetString("Common/close")))
			{
				return GetCharactorList(gameCode, token, sn, appVersion);
			}
			else
			{
				MainWindow::Get()->AppClose();
			}
			break;
	}
	return false;
}

bool LimeCommunicator::CreateChatting(const char* gameUserID, const char* token, const char* deviceId, eServiceNetwork sn, const char* appVersion, const char* params)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::CreateChatting]");

	json_error_t error;
	json_t *json_result;
	std::string lime_token = GetLimeToken(gameUserID, token, deviceId, sn, appVersion);
	if (lime_token.empty())
		return false;

	if (::CreateChatting(lime_token.c_str(), m_vtrCharacters, sn, appVersion, &json_result, error, params))
	{
		OnCreateChatting(json_result);
	}
	else
	{
		LOG(ERROR) << format_string("[LimeCommunicator::CreateChatting] error(%s)", error.text);
		VideoChatMessageBox::Instance()->information(AppString::GetString("error_message/error_lime_create_chatting"));
		return false;
	}

	return true;
}

bool LimeCommunicator::GetGuildUserList(const char* gameUserID, const char* token, const char* deviceId, eServiceNetwork sn, const char* appVersion)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::GetGuildUserList]");

	json_error_t error;
	json_t *json_result;
	std::string lime_token = GetLimeToken(gameUserID, token, deviceId, sn, appVersion);
	if (lime_token.empty())
		return false;

	m_guildUserInfoList.guildUserInfoList.clear();
	
	if (::GetInvitationUserList(lime_token.c_str(), m_vtrCharacters, sn, appVersion, &json_result, error, gameUserID))
	{
		OnGetInvitationUserList(json_result);
	}

	else
	{
		LOG(ERROR) << format_string("[LimeCommunicator::GetGuildUserList] error(%d)", error.text);
		if (VideoChatMessageBox::Instance()->selection(AppString::GetString(AppUtils::format("%s/error_lime_get_guild_user_list", App()->GetOriginGameCode().c_str()).c_str()), 
			AppString::GetString("Common/retry"), 
			AppString::GetString("Common/close")))
		{
			return GetGuildUserList(gameUserID, token, deviceId, sn, appVersion);
		}
		else
		{
			MainWindow::Get()->AppClose();
		}
		return false;
	}

	return true;
}

GuildUserInfoList& LimeCommunicator::GetGuildUserList()
{
	return m_guildUserInfoList;
}

ChattingInfo& LimeCommunicator::GetCreateChatInfo()
{
	return m_chattingInfo;
}

bool LimeCommunicator::GetShareTargetList(const char* gameUserID, const char* token, const char* deviceId, eServiceNetwork sn, const char* appVersion)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::GetShareTargetList]");

	json_error_t error;
	json_t *json_result;
	std::string lime_token = GetLimeToken(gameUserID, token, deviceId, sn, appVersion);
	if (lime_token.empty())
		return false;

	if (!::GetShareTargetList(lime_token.c_str(), m_vtrCharacters, sn, appVersion, &json_result, error))
	{
		LOG(ERROR) << format_string("[LimeCommunicator::GetShareTargetList] error(%s)", error.text);
		if (VideoChatMessageBox::Instance()->selection(AppString::GetString("error_message/error_lime_get_share_target_list"), AppString::GetString("Common/retry"), AppString::GetString("Common/close")))
		{
			return GetShareTargetList(gameUserID, token, deviceId, sn, appVersion);
		}
		else
		{
			MainWindow::Get()->AppClose();
		}
		return false;
	}

	OnGetShareTargetList(json_result);
	return true;
}

void LimeCommunicator::CheckRetryConnection()
{
	elogd("[LimeCommunicator::CheckRetryConnect]");
	if (!m_retryConnectionHelper.IsRetrying())
	{
		// LIME 재 연결 시도 가이드라인
		// 접속이 단절되면 2초 뒤에 접속 재시도를 합니다.
		// 이후에도 연결되지 않으면 4초, 8초, 16초, 32초등 재시도 간격을 2배씩 늘려 재시도합니다.
		// 단 재시도 간격은 최대 256초를 넘지 않게 합니다.
		m_retryConnectionHelper.SetRetryTimes({ 2, 4, 8, 16, 32, 64, 128, 256 });
		m_retryConnectionHelper.Init();
		SetRetryConnectionState(eRetryConnectionState::eRCS_Retrying);
	}

	m_retryConnectionHelper.Retry([this](bool timeout) {
		if (timeout)
			SetRetryConnectionState(eRetryConnectionState::eRCS_Failed);
		else
			ReconnectLiveWebServer();
	});
}

void LimeCommunicator::StopRetryConnection()
{
	elogd("[LimeCommunicator::StopRetryConnect]");
	if (m_retryConnectionHelper.IsRetrying())
		m_retryConnectionHelper.Stop();
}

void LimeCommunicator::ReconnectLiveWebServer()
{
	eloge("[LimeCommunicator::ReconnectLiveWebServer]");

	if (!GetSubscriptionInfo())
	{
		LOG(ERROR) << format_string("[LimeCommunicator::ReconnectLiveWebServer] RefreshSubscriptionInfo failed");
		CheckRetryConnection();
		return;
	}

	ConnectLiveWebServer();
}

void LimeCommunicator::ConnectLiveWebServer()
{
	m_pStompClient = std::make_unique<QTWebStompClient>(m_stompInfo.subscribeURL.c_str(), m_stompInfo.login.c_str(), m_stompInfo.passcode.c_str(), this, m_stompInfo.jwt.c_str());

	LOG(DEBUG) << format_string("[LimeCommunicator::ConnectLiveWebServer] STOMP connect url(%s) login(%s) passcode(%s)", m_stompInfo.subscribeURL.c_str(), m_stompInfo.login.c_str(), m_stompInfo.passcode.c_str());

	m_pStompClient->ConnectSocket();
}

eRetryConnectionState LimeCommunicator::GetRetryConnectionState()
{
	return m_retryConnectionState;
}

eConnectionState LimeCommunicator::GetConnectionState()
{
	if (m_stompInfo.jwt == "")
	{
		return eConnectionState::eCS_None;
	}
	else
	{
		if (m_stompInfo.subscribeURL == "")
			return eConnectionState::eCS_NeedLogIn;
		else
		{
			if (!m_pStompClient)
				return eConnectionState::eCS_NeedConnect;
			else
			{
				QTWebStompClient::ConnectionState state = m_pStompClient->GetConnectionState();
				switch (state)
				{
				case QTWebStompClient::Closed:
				{
					return eConnectionState::eCS_NeedConnect;
				}
				break;
				case QTWebStompClient::Connected:
				case QTWebStompClient::Subscribed:
				case QTWebStompClient::Connecting:
				default:
					return eConnectionState::eCS_ConnectedAll;
					break;
				}
			}
		}
	}
}

eStreamingState LimeCommunicator::GetStreamingState()
{
	return m_streamingState;
}

void LimeCommunicator::SetStreamingState(eStreamingState state)
{
	LOG(INFO) << format_string("[LimeCommunicator::SetStreamingState] eStreamingState(%d)", state);
	if (m_streamingState == state)
		return;

	m_streamingState = state;
	MainWindow::Get()->OnChangeStreamingState(m_streamingState);
}

void LimeCommunicator::SetRetryConnectionState(eRetryConnectionState state)
{
	LOG(INFO) << format_string("[LimeCommunicator::SetRetryConnectState] eRetryConnectState(%d)", state);
	if (m_retryConnectionState == state)
		return;

	switch (state)
	{
	case eRetryConnectionState::eRCS_None:
	case eRetryConnectionState::eRCS_Retrying:
	case eRetryConnectionState::eRCS_Successed:
		break;
	case eRetryConnectionState::eRCS_Failed:
		AppUtils::DispatchToMainThread([this] {
			MainWindow::Get()->AppClose(AppString::GetString("error_message/error_lime_retry_failed").c_str());
		});
		break;
	default:
		break;
	}

	m_retryConnectionState = state;
}

void LimeCommunicator::ConnectedProcess(bool reconnect)
{
	LOG(INFO) << format_string("[LimeCommunicator::ConnectedProcess] reconnect(%d)", reconnect);

	MainWindow::Get()->OnPubServerConnected(reconnect);

	bool streaming = GetStreamingState()==eStreamingState::eSS_Streaming;
	if (reconnect && streaming)
	{
		Send_ReconnectRoom();
	}
}

bool LimeCommunicator::ConnectPubServer(const char* gameUserID, const char* token, const char* deviceId, eServiceNetwork sn, const char* appVersion)
{
	eConnectionState state = GetConnectionState();

	switch (state)
	{
	case eConnectionState::eCS_None:
	case eConnectionState::eCS_NeedLogIn:
	{
		std::string lime_token = GetLimeToken(gameUserID, token, deviceId, sn, appVersion);
		if (lime_token.empty())
			return false;
		ConnectLiveWebServer();
	}
	break;
	case eConnectionState::eCS_NeedConnect:
		ConnectLiveWebServer();
		break;
	case eConnectionState::eCS_ConnectedAll:
		ConnectedProcess();
		break;
	}

	return true;
}

void LimeCommunicator::RequestStartStreaming()
{
	if (m_isForceExcute)
	{
		if (m_roomId.empty())
		{
			LOG(DEBUG) << "[LimeCommunicator::ConnectedProcess] [error] need m_roomId";
			SetFirstShareTarget();
			Send_CreateRoom();
		}
		else
		{
			Send_JoinRoom(m_roomId);
		}
	}
	else
	{
		if (m_groupUserId.empty() || m_channelId.empty())
		{
			LOG(DEBUG) << "[LimeCommunicator::ConnectedProcess] [error] need m_groupUserId, m_channelId";
			SetFirstShareTarget();
			Send_CreateRoom();
		}
		else
		{
			Send_CreateRoom();
		}
	}
}

void LimeCommunicator::RequestStopStreaming()
{
	SetStreamingState(eStreamingState::eSS_Stopping);
	Send_StopStream();
}

void LimeCommunicator::RequestCloseRoom()
{
	Send_CloseRoom();
}

void LimeCommunicator::RequestCreateChatting(const char* params)
{
	Send_CreateChatting(params);
}

void LimeCommunicator::RequestGuildUserList()
{
	m_guildUserInfoList.guildUserInfoList.clear();
	Send_GetGuildUserList();
}

void LimeCommunicator::RequestShareTargetList()
{
	Send_GetShareTargetList();
}

bool LimeCommunicator::RequestRoomUserList(bool isAppend, int count)
{
	if (GetStreamingState()!=eStreamingState::eSS_Streaming)
		return false;

	if (!isAppend)
	{
		m_roomUserInfoList.viewerList.clear();
	}

	Send_GetRoomUserList(m_roomId, m_roomUserInfoList.viewerList.size(), count);
	return true;
}

bool LimeCommunicator::RequestWatchingUserList(bool isAppend, int count)
{
	if (GetStreamingState() != eStreamingState::eSS_Streaming)
		return false;

	if (!isAppend)
	{
		m_roomUserInfoList.viewerList.clear();
	}

	Send_GetWatchingUserList(m_roomId, m_roomUserInfoList.viewerList.size(), count);
	return true;
}

void LimeCommunicator::RequestDeportRoom(const DeportUserInfo& info)
{
	Send_DeportRoom(m_roomId, info);
}

bool LimeCommunicator::IsLogin()
{
	eConnectionState state = GetConnectionState();
	switch (state)
	{
		case eConnectionState::eCS_None:
		case eConnectionState::eCS_NeedLogIn:
			return false;
		case eConnectionState::eCS_NeedConnect:
		case eConnectionState::eCS_ConnectedAll:
			return true;
		default:
			return false;
	}
}

bool LimeCommunicator::Relogin()
{
	Logout();
	return LoginWithJWToken(m_gameUserID.c_str(), m_token.c_str(), m_deviceId.c_str(), m_stompInfo, m_serviceNetwork, m_appVersion.c_str(), App()->GetLocalLanguage().toStdString().c_str());
}

bool LimeCommunicator::Login(const char* gameUserID, const char* token, const char* deviceId, eServiceNetwork sn, const char* appVersion)
{
	m_gameUserID = gameUserID;
	m_token = token;
	m_deviceId = deviceId;
	m_serviceNetwork = sn;
	m_appVersion = appVersion;
	return LoginWithJWToken(m_gameUserID.c_str(), m_token.c_str(), m_deviceId.c_str(), m_stompInfo, m_serviceNetwork, m_appVersion.c_str(), App()->GetLocalLanguage().toStdString().c_str());
}

void LimeCommunicator::Logout()
{
	m_stompInfo.InitValue();
}

bool LimeCommunicator::GetSubscriptionInfo()
{
	std::string lime_token = GetLimeToken(m_gameUserID.c_str(), m_token.c_str(), m_deviceId.c_str(), m_serviceNetwork, m_appVersion.c_str());
	if (lime_token.empty())
		return false;

	return ::GetSubscriptionInfo(lime_token.c_str(), m_stompInfo, m_serviceNetwork, m_appVersion.c_str(), App()->GetLocalLanguage().toStdString().c_str());
}

void LimeCommunicator::ClearLimeToken()
{
	m_limeToken = "";
}

std::string LimeCommunicator::GetLimeToken(const char* gameUserID, const char* token, const char* deviceId, eServiceNetwork sn, const char* appVersion)
{
	if (!m_limeToken.empty())
		return m_limeToken;

	if (!Login(gameUserID, token, deviceId, sn, appVersion)) 
	{
		LOG(ERROR) << format_string("[LimeCommunicator::GetLimeToken] Login failed");
		if (VideoChatMessageBox::Instance()->selection(AppString::GetString("error_message/error_lime_login_purple"), AppString::GetString("Common/retry"), AppString::GetString("Common/close")))
		{
			return GetLimeToken(gameUserID, token, deviceId, sn, appVersion);
		}
		else
		{
			MainWindow::Get()->AppClose();
		}
		m_limeToken = "";
		return m_limeToken;
	}

	m_limeToken = m_stompInfo.jwt;
	return m_limeToken;
}

void LimeCommunicator::OnConnectedWebSocket()
{
	m_pStompClient->ConnectStomp();
}

void LimeCommunicator::OnConnected()
{
	bool reconnect = GetRetryConnectionState() == eRetryConnectionState::eRCS_Retrying;
	if (reconnect)
	{
		SetRetryConnectionState(eRetryConnectionState::eRCS_Successed);
		StopRetryConnection();
	}

	m_pStompClient->Subscribe(m_stompInfo.topicName.c_str());

	ConnectedProcess(reconnect);
}

void LimeCommunicator::OnDisconnected()
{
	LOG(DEBUG) << format_string("[LimeCommunicator::OnDisconnected]");

	AppUtils::DispatchToMainThread([this] {
		CheckRetryConnection();
	});
}

void LimeCommunicator::OnMessage(const StompMessage &msg)
{
	string readBuffer = msg.m_message;
	json_error_t error;
	json_t *root = json_loads(readBuffer.c_str(), 0, &error);

	if (!root)
	{
		eloge("error: on line %d: %s\n", error.line, error.text);
		return;
	}

	// parse json header -------------------------------------------
	string strTid = json_get_string_value(root, "tid");
	string strType = json_get_string_value(root, "type");
	string strMethod = json_get_string_value(root, "method");

	json_t* jsonData = json_object_get(root, "jsonData");
	// oncloseroom에서는 jsonData가 없음(NULL)
/*	if (!json_is_object(jsonData))
	{
		json_decref(root);
		return;
	}
*/
	if (strType == "ERROR")
	{
		LOG(ERROR) << format_string("[LimeCommunicator::OnMessage] type(%s) method(%s)", strType.c_str(), strMethod.c_str());

		json_t* viewMessage = json_object_get(jsonData, "viewMessage");
		if (json_is_object(viewMessage))
		{
			string strErrorMsg = json_get_string_value(viewMessage, "message");
			VideoChatMessageBox::Instance()->information(strErrorMsg);
		}

		MainWindow::Get()->OnError();
			
		if (strMethod == "/stream/updateRoom")
		{
			OnUpdateRoom(jsonData, false);
			VideoChatMessageBox::Instance()->information(AppString::GetString("error_message/error_lime_update_room"));
		}
		else if (strMethod == "/stream/startStream")
		{
			OnStartStream(jsonData, false);
			VideoChatMessageBox::Instance()->information(AppString::GetString("error_message/error_lime_start_stream"));
		}
		else if (strMethod == "/stream/stopStream")
		{
			OnStopStream(jsonData, false);
			VideoChatMessageBox::Instance()->information(AppString::GetString("error_message/error_lime_stop_stream"));
		}
		else if (strMethod == "/stream/pauseStream")
		{
			OnPauseStream(jsonData, false);
			VideoChatMessageBox::Instance()->information(AppString::GetString("error_message/error_lime_pause_stream"));
		}
		else if (strMethod == "/stream/reconnectRoom")
		{
			OnReconnectRoom(jsonData, false);
		}
		else if (strMethod == "/stream/createRoom")
		{
			OnCreateRoom(jsonData, false);
			VideoChatMessageBox::Instance()->information(AppString::GetString("error_message/error_lime_create_room"));
		}
		else if (strMethod == "/stream/joinRoom")
		{
			OnJoinRoom(jsonData, false);
			VideoChatMessageBox::Instance()->information(AppString::GetString("error_message/error_lime_join_room"));
		}
		else if (strMethod == "/stream/closeRoom")
		{
			OnCreateRoom(jsonData, false);
			VideoChatMessageBox::Instance()->information(AppString::GetString("error_message/error_lime_close_room"));
		}
		else if (strMethod == "/stream/deportRoom")
		{
			OnDeportRoom(jsonData, false);
			VideoChatMessageBox::Instance()->information(AppString::GetString("error_message/error_lime_deport_room"));
		}
		else if (strMethod == "/stream/chat/createChatting")
		{
			OnCreateChatting(jsonData, false);
		}
		else if (strMethod == "/stream/chat/getGuildUserList")
		{
			OnGetGuildUserList(jsonData, false);
		}
		else if (strMethod == "/stream/getShareTargetList")
		{
			OnGetShareTargetList(jsonData, false);
		}
		return;
	}

	if (strType == "RESPONSE")
	{
		LOG(DEBUG) << format_string("[LimeCommunicator::OnMessage] type(%s) method(%s)", strType.c_str(), strMethod.c_str());

		if (strMethod == "/stream/chat/createChatting")
		{
			OnCreateChatting(jsonData);
		}
		else if (strMethod == "/stream/chat/getGuildUserList")
		{
			OnGetGuildUserList(jsonData);
		}
		else if (strMethod == "/stream/getUnclosedRoomInfo")
		{
			OnGetUnclosedRoomInfo(jsonData);
		}
		else if (strMethod == "/stream/getShareTargetList")
		{
			OnGetShareTargetList(jsonData);
		}
		else if (strMethod == "/stream/getRoomUserList")
		{
			OnGetRoomUserList(jsonData);
		}
		else if (strMethod == "/stream/getWatchingUserList")
		{
			OnGetWatchingUserList(jsonData);
		}
		else if (strMethod == "/stream/deportRoom")
		{
			OnDeportRoom(jsonData);
		}
		else if (strMethod == "/stream/reconnectRoom")
		{
			OnReconnectRoom(jsonData);
		}
		else if (strMethod == "/stream/createRoom")
		{
			OnCreateRoom(jsonData);
		}
		else if (strMethod == "/stream/joinRoom")
		{
			OnJoinRoom(jsonData);
		}
		else if (strMethod == "/stream/startStream")
		{
			// MAP logging(LogStartStream)
			MAPManager::Instance()->LogStartStream(false, false);
			OnStartStream(jsonData);
		}
		else if (strMethod == "/stream/stopStream")
		{
			// MAP logging(LogStopStream)
			MAPManager::Instance()->LogStopStream(false, false);
			OnStopStream(jsonData);
		}
		else if (strMethod == "/stream/pauseStream")
		{
			// MAP logging(LogStopStream)
			MAPManager::Instance()->LogStopStream(false, false);
			OnPauseStream(jsonData);
		}
		else if (strMethod == "/stream/closeRoom")
		{
			OnCloseRoom(jsonData);
		}
		else if (strMethod == "/stream/updateRoom")
		{
			OnUpdateRoom(jsonData);
		}
	}
	else if (strType == "MESSAGE")
	{
		/*
		메시지 전송 /stream/publish PUBLISH NORMAL
		귓속말 전송 /stream/publish PUBLISH WHISPER
		NEMO 이모티콘 전송 /stream/publish NEMO NORMAL
		*/
		if (strMethod == "/stream/publish")
		{
			
		}
	}
	else if (strType == "EVENT")
	{
		/*
		방 입장					/event/stream/room/join				STREAM_ROOM_JOIN
		방 퇴장					/event/stream/room/leave			STREAM_ROOM_LEAVE
		방 종료					/event/stream/room/close			STREAM_ROOM_CLOSE
		방 추방					/event/stream/room/deport			STREAM_ROOM_DEPORT
		방 추방 해제			/event/stream/room/undeport			STREAM_ROOM_UNDEPORT
		방 정보 변경			/event/stream/room/update			STREAM_ROOM_UPDATE
		채팅 금지				/event/stream/room/ban				STREAM_ROOM_BAN
		채팅 금지 해제			/event/stream/room/unban			STREAM_ROOM_UNBAN
		방송 시작				/event/stream/streaming/start			STREAM_STREAMING_START
		방송 재개				/event/stream/streaming/resume			STREAM_STREAMING_RESUME
		방송 일시 정지			/event/stream/streaming/pause			STREAM_STREAMING_PAUSE
		방송 종료				/event/stream/streaming/stop			STREAM_STREAMING_STOP
		방송 중단(사고)			/event/stream/streaming/accident		STREAM_STREAMING_ACCIDENT
		시청/조회 수 업데이트	/event/stream/streaming/count			STREAM_STREAMING_COUNT
		길드원 방송 시작		/event/stream/guild/member/streaming/start	STREAM_GUILD_MEMBER_STREAMING_START
		방송 강제 종료			/event/stream/streaming/forceQuit		STREAM_STREAMING_FORCE_QUIT
		화면 공유 시작			/event/stream/streaming/share/start		STREAM_STREAMING_SHARE_START
		화면 공유 종료			/event/stream/streaming/share/stop		STREAM_STREAMING_SHARE_STOP
		그룹 권한 삭제			/event/stream/chat/group/leave			STREAM_CHAT_GROUP_LEAVE
		*/
		if (strMethod == "/event/stream/room/join")
		{
			OnEventJoin(jsonData);
		}
		else if (strMethod == "/event/stream/room/leave")
		{
			OnEventLeave(jsonData);
		}
		else if (strMethod == "/event/stream/room/deport")
		{
			OnEventDeport(jsonData);
		}
		else if (strMethod == "/event/stream/streaming/count")
		{
			OnEventCount(jsonData);
		}
		else if (strMethod == "/event/stream/chat/group/leave")
		{
			OnEventChatGroupLeave(jsonData);
		}
		else if (strMethod == "/event/stream/streaming/forceQuit")
		{
			OnEventStopStream(jsonData, true);
		}
		else if (strMethod == "/event/stream/streaming/stop" ||
			strMethod == "/event/stream/room/close")
		{
			OnEventStopStream(jsonData, false);
		}
	}
	else if (strType == "NOTI")
	{
	}
	else if (strType == "SYSTEM")
	{
	}
}

void LimeCommunicator::OnErrorMessage(const char* message)
{
	LOG(ERROR) << format_string("[LimeCommunicator::OnMessageError] message(%d)", message);
	std::string msg(message);
	AppUtils::DispatchToMainThread([msg] {
		MainWindow::Get()->AppClose(AppUtils::format(AppString::GetString("error_message/error_lime_stomp_state").c_str(), msg.c_str()).c_str());
	});
}

void LimeCommunicator::OnError(QAbstractSocket::SocketError error)
{
	LOG(ERROR) << format_string("[LimeCommunicator::OnError] error(%d)", error);

	MainWindow::Get()->OnError();

	switch (error)
	{
	case QAbstractSocket::RemoteHostClosedError:
		//The remote host closed the connection.Note that the client socket(i.e., this socket) will be closed after the remote close notification has been sent.
		// 방송 중에 연결이 끊긴 경우
		AppUtils::DispatchToMainThread([this] {
			CheckRetryConnection();
		});
		return;
	case QAbstractSocket::ProxyProtocolError:
		//The connection negotiation with the proxy server failed, because the response from the proxy server could not be understood.
		// Pub(STOMP) 서버에 연결할 수 없는 경우
		//		1) Pub 연결 재시도 중이면 재시도 처리
		//		2) Pub 연결 재시도 중이 아니면 오류 메시지 후 종료 처리
		if (GetRetryConnectionState() == eRetryConnectionState::eRCS_Retrying)
		{
			AppUtils::DispatchToMainThread([this] {
				CheckRetryConnection();
			});
			return;
		}
	case QAbstractSocket::ConnectionRefusedError:
		//The connection was refused by the peer(or timed out).
	case QAbstractSocket::HostNotFoundError:
		//The host address was not found.
	case QAbstractSocket::SocketAccessError:
		//The socket operation failed because the application lacked the required privileges.
	case QAbstractSocket::SocketResourceError:
		//The local system ran out of resources(e.g., too many sockets).
	case QAbstractSocket::SocketTimeoutError:
		//The socket operation timed out.
	case QAbstractSocket::DatagramTooLargeError:
		//The datagram was larger than the operating system's limit (which can be as low as 8192 bytes).
	case QAbstractSocket::NetworkError:
		//An error occurred with the network(e.g., the network cable was accidentally plugged out).
	case QAbstractSocket::AddressInUseError:
		//The address specified to QAbstractSocket::bind() is already in use and was set to be exclusive.
	case QAbstractSocket::SocketAddressNotAvailableError:
		//The address specified to QAbstractSocket::bind() does not belong to the host.
	case QAbstractSocket::UnsupportedSocketOperationError:
		//The requested socket operation is not supported by the local operating system(e.g., lack of IPv6 support).
	case QAbstractSocket::ProxyAuthenticationRequiredError:
		//The socket is using a proxy, and the proxy requires authentication.
	case QAbstractSocket::SslHandshakeFailedError:
		//The SSL / TLS handshake failed, so the connection was closed(only used in QSslSocket)
	case QAbstractSocket::UnfinishedSocketOperationError:
		//Used by QAbstractSocketEngine only, The last operation attempted has not finished yet(still in progress in the background).
	case QAbstractSocket::ProxyConnectionRefusedError:
		// Could not contact the proxy server because the connection to that server was denied
	case QAbstractSocket::ProxyConnectionClosedError:
		//The connection to the proxy server was closed unexpectedly(before the connection to the final peer was established)
	case QAbstractSocket::ProxyConnectionTimeoutError:
		//The connection to the proxy server timed out or the proxy server stopped responding in the authentication phase.
	case QAbstractSocket::ProxyNotFoundError:
		//The proxy address set with setProxy() (or the application proxy) was not found.
	case QAbstractSocket::OperationError:
		//An operation was attempted while the socket was in a state that did not permit it.
	case QAbstractSocket::SslInternalError:
		//The SSL library being used reported an internal error.This is probably the result of a bad installation or misconfiguration of the library.
	case QAbstractSocket::SslInvalidUserDataError:
		//Invalid data(certificate, key, cypher, etc.) was provided and its use resulted in an error in the SSL library.
	case QAbstractSocket::TemporaryError:
		//A temporary error occurred(e.g., operation would block and socket is non - blocking).
	case QAbstractSocket::UnknownSocketError:
		//An unidentified error occurred.
	default:
		AppUtils::DispatchToMainThread([error] {
			MainWindow::Get()->AppClose(AppUtils::format(AppString::GetString("error_message/error_lime_server").c_str(), error).c_str());
		});
		break;
	}

	SetStreamingState(eStreamingState::eSS_Error);
}

void LimeCommunicator::Send_CreateChatting(const char* params)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::Send_GetGuildUserList]");

	string tid = generate_lime_trace_id();

	json_t* root = json_object();

	json_object_set(root, "tid", json_string(tid.c_str()));
	json_object_set(root, "method", json_string("/stream/chat/createChatting"));
	
	json_error_t error;
	json_t *json_params = json_loads(params, 0, &error);
	if (!json_params)
		json_params = json_object();
	json_object_set_new(root, "params", json_params);

	string message = json_dumps(root, 0);

	m_pStompClient->Send(m_stompInfo.serverAppDest.c_str(), tid, message, App()->GetLocalLanguage().toStdString());
}

void LimeCommunicator::Send_GetGuildUserList()
{
	LOG(DEBUG) << format_string("[LimeCommunicator::Send_GetGuildUserList]");

	string tid = generate_lime_trace_id();

	json_t* root = json_object();

	json_object_set(root, "tid", json_string(tid.c_str()));
	json_object_set(root, "method", json_string("/stream/chat/getGuildUserList"));
	
	string message = json_dumps(root, 0);

	m_pStompClient->Send(m_stompInfo.serverAppDest.c_str(), tid, message, App()->GetLocalLanguage().toStdString());
}

void LimeCommunicator::Send_GetShareTargetList()
{
	LOG(DEBUG) << format_string("[LimeCommunicator::Send_GetShareTargetList]");

	string tid = generate_lime_trace_id();

	json_t* root = json_object();

	json_object_set(root, "tid", json_string(tid.c_str()));
	json_object_set(root, "method", json_string("/stream/getShareTargetList"));
	
	string message = json_dumps(root, 0);

	m_pStompClient->Send(m_stompInfo.serverAppDest.c_str(), tid, message, App()->GetLocalLanguage().toStdString());
}

void LimeCommunicator::Send_GetRoomUserList(string roomId, int cursor, int count/*(default 100, MAX 100)*/)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::Send_GetRoomUserList]");

	string tid = generate_lime_trace_id();

	json_t* root = json_object();

	json_object_set(root, "tid", json_string(tid.c_str()));
	json_object_set(root, "method", json_string("/stream/getRoomUserList"));
	json_object_set_new(root, "params", json_object());

	json_t* body = json_object_get(root, "params");

	json_object_set(body, "roomId", json_string(roomId.c_str()));
	json_object_set(body, "cursor", json_string(AppUtils::format("%d", cursor).c_str()));
	json_object_set(body, "maxListCount", json_string(AppUtils::format("%d", count).c_str()));

	string message = json_dumps(root, 0);

	m_pStompClient->Send(m_stompInfo.serverAppDest.c_str(), tid, message, App()->GetLocalLanguage().toStdString());
}

void LimeCommunicator::Send_GetWatchingUserList(string roomId, int cursor, int count/*(default 100, MAX 100)*/)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::Send_GetWatchingUserList]");

	string tid = generate_lime_trace_id();

	json_t* root = json_object();

	json_object_set(root, "tid", json_string(tid.c_str()));
	json_object_set(root, "method", json_string("/stream/getWatchingUserList"));
	json_object_set_new(root, "params", json_object());

	json_t* body = json_object_get(root, "params");

	json_object_set(body, "roomId", json_string(roomId.c_str()));
	json_object_set(body, "cursor", json_string(AppUtils::format("%d", cursor).c_str()));
	json_object_set(body, "maxListCount", json_string(AppUtils::format("%d", count).c_str()));

	string message = json_dumps(root, 0);

	m_pStompClient->Send(m_stompInfo.serverAppDest.c_str(), tid, message, App()->GetLocalLanguage().toStdString());
}

void LimeCommunicator::Send_DeportRoom(string roomId, const DeportUserInfo& info)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::Send_DeportRoom]");

	string tid = generate_lime_trace_id();

	json_t* root = json_object();

	json_object_set(root, "tid", json_string(tid.c_str()));
	json_object_set(root, "method", json_string("/stream/deportRoom"));
	json_object_set_new(root, "params", json_object());
	json_t* params = json_object_get(root, "params");
	{
		json_object_set(params, "roomId", json_string(roomId.c_str()));
		json_object_set_new(params, "deportUser", json_object());
		json_t* deportUser = json_object_get(params, "deportUser");
		{
			json_object_set(deportUser, "gameUserId", json_string(info.gameUserId.c_str()));
			json_object_set(deportUser, "deport", (info.deport) ? json_true() : json_false());
		}
		json_object_set(params, "deportUsers", json_null());
	}

	string message = json_dumps(root, 0);

	m_pStompClient->Send(m_stompInfo.serverAppDest.c_str(), tid, message, App()->GetLocalLanguage().toStdString());
}

void LimeCommunicator::Send_GetUnclosedRoomInfo()
{
	LOG(DEBUG) << format_string("[LimeCommunicator::Send_GetUnclosedRoomInfo]");

	string tid = generate_lime_trace_id();

	json_t* root = json_object();

	json_object_set(root, "tid", json_string(tid.c_str()));
	json_object_set(root, "method", json_string("/stream/getUnclosedRoomInfo"));
	json_object_set_new(root, "params", json_object());

	json_t* body = json_object_get(root, "params");

	json_object_set(body, "roomKey", json_string(""));							// password
	json_object_set(body, "name", json_string(m_roomName.c_str()));				// 방제
	json_object_set(body, "description", json_string(""));						// 
	json_object_set(body, "type", json_string("PUBLIC"));						// 공개범위(전체, 길드)
	json_object_set(body, "dateReserved", json_string(""));
	json_object_set(body, "dateStreamStart", json_string(""));
	json_object_set(body, "additionalInfo1", json_string(""));
	json_object_set(body, "additionalInfo2", json_string(""));

	string message = json_dumps(root, 0);

	m_pStompClient->Send(m_stompInfo.serverAppDest.c_str(), tid, message, App()->GetLocalLanguage().toStdString());
}

void LimeCommunicator::Send_ReconnectRoom()
{
	LOG(DEBUG) << format_string("[LimeCommunicator::Send_ReconnectRoom]");

	string tid = generate_lime_trace_id();

	json_t* root = json_object();

	json_object_set(root, "tid", json_string(tid.c_str()));
	json_object_set(root, "method", json_string("/stream/reconnectRoom"));
	json_object_set_new(root, "params", json_object());

	json_t* body = json_object_get(root, "params");

	json_object_set(body, "roomId", json_string(m_roomId.c_str()));

	string message = json_dumps(root, 0);

	m_pStompClient->Send(m_stompInfo.serverAppDest.c_str(), tid, message, App()->GetLocalLanguage().toStdString());
}

void LimeCommunicator::Send_CreateRoom()
{
	LOG(DEBUG) << format_string("[LimeCommunicator::Send_CreateRoom]");

	string tid = generate_lime_trace_id();

	json_t* root = json_object();

	json_object_set(root, "tid", json_string(tid.c_str()));
	json_object_set(root, "method", json_string("/stream/createRoom"));
	json_object_set_new(root, "params", json_object());

	json_t* body = json_object_get(root, "params");

	json_object_set(body, "roomKey", json_string(""));								// password
	json_object_set(body, "name", json_string(m_roomName.c_str()));					// 방제
	json_object_set(body, "description", json_string(""));
	json_object_set(body, "type", json_string("VIDEO_SHARE"));
	json_object_set(body, "dateReserved", json_string(""));
	json_object_set(body, "dateStreamStart", json_string(""));
	json_object_set(body, "additionalInfo1", json_string(""));
	json_object_set(body, "additionalInfo2", json_string(""));
	json_object_set(body, "groupUserId", json_string(m_groupUserId.c_str()));
	json_object_set(body, "channelId", json_string(m_channelId.c_str()));

	string message = json_dumps(root, 0);

	m_pStompClient->Send(m_stompInfo.serverAppDest.c_str(), tid, message, App()->GetLocalLanguage().toStdString());
}

void LimeCommunicator::Send_JoinRoom(string roomId)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::Send_JoinRoom]");

	string tid = generate_lime_trace_id();

	json_t* root = json_object();

	json_object_set(root, "tid", json_string(tid.c_str()));
	json_object_set(root, "method", json_string("/stream/joinRoom"));
	json_object_set_new(root, "params", json_object());

	json_t* body = json_object_get(root, "params");

	json_object_set(body, "roomId", json_string(roomId.c_str()));
	json_object_set(body, "roomKey", json_string(""));

	string message = json_dumps(root, 0);

	m_pStompClient->Send(m_stompInfo.serverAppDest.c_str(), tid, message, App()->GetLocalLanguage().toStdString());
}

void LimeCommunicator::Send_StartStream(string roomId)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::Send_StartStream]");

	string tid = generate_lime_trace_id();

	json_t* root = json_object();

	json_object_set(root, "tid", json_string(tid.c_str()));
	json_object_set(root, "method", json_string("/stream/startStream"));
	json_object_set_new(root, "params", json_object());

	json_t* body = json_object_get(root, "params");

	json_object_set(body, "roomId", json_string(roomId.c_str()));

	string message = json_dumps(root, 0);

	m_pStompClient->Send(m_stompInfo.serverAppDest.c_str(), tid, message, App()->GetLocalLanguage().toStdString());
}

void LimeCommunicator::Send_StopStream()
{
	LOG(DEBUG) << format_string("[LimeCommunicator::Send_StopStream]");

	string tid = generate_lime_trace_id();

	json_t* root = json_object();

	json_object_set(root, "tid", json_string(tid.c_str()));
	json_object_set(root, "method", json_string("/stream/stopStream"));
	json_object_set_new(root, "params", json_object());

	json_t* body = json_object_get(root, "params");

	json_object_set(body, "roomId", json_string(m_roomId.c_str()));

	string message = json_dumps(root, 0);

	m_pStompClient->Send(m_stompInfo.serverAppDest.c_str(), tid, message, App()->GetLocalLanguage().toStdString());
}

void LimeCommunicator::Send_PauseStream()
{
	LOG(DEBUG) << format_string("[LimeCommunicator::Send_PauseStream]");

	string tid = generate_lime_trace_id();

	json_t* root = json_object();

	json_object_set(root, "tid", json_string(tid.c_str()));
	json_object_set(root, "method", json_string("/stream/pauseStream"));
	json_object_set_new(root, "params", json_object());

	json_t* body = json_object_get(root, "params");

	json_object_set(body, "roomId", json_string(m_roomId.c_str()));

	string message = json_dumps(root, 0);

	m_pStompClient->Send(m_stompInfo.serverAppDest.c_str(), tid, message, App()->GetLocalLanguage().toStdString());
}

void LimeCommunicator::Send_CloseRoom()
{
	LOG(DEBUG) << format_string("[LimeCommunicator::Send_CloseRoom]");

	string tid = generate_lime_trace_id();

	json_t* root = json_object();

	json_object_set(root, "tid", json_string(tid.c_str()));
	json_object_set(root, "method", json_string("/stream/closeRoom"));
	json_object_set_new(root, "params", json_object());

	json_t* body = json_object_get(root, "params");

	json_object_set(body, "roomId", json_string(m_roomId.c_str()));

	string message = json_dumps(root, 0);

	m_pStompClient->Send(m_stompInfo.serverAppDest.c_str(), tid, message, App()->GetLocalLanguage().toStdString());
}

void LimeCommunicator::Send_UpdateRoom(const string roomName)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::Send_UpdateRoom]");

	string tid = generate_lime_trace_id();

	json_t* root = json_object();

	json_object_set(root, "tid", json_string(tid.c_str()));
	json_object_set(root, "method", json_string("/stream/updateRoom"));
	json_object_set_new(root, "params", json_object());

	json_t* body = json_object_get(root, "params");

	json_object_set(body, "roomId", json_string(m_roomId.c_str()));
	string newRoomName = roomName;
	string name = StringUtils::trim(newRoomName);
	json_object_set(body, "name", json_string(name.c_str()));					// 방제
	json_object_set(body, "description", json_string(""));						// 
	json_object_set(body, "type", json_string("PUBLIC"));						// 공개범위(전체, 길드)
	json_object_set(body, "dateReserved", json_string(""));
	json_object_set(body, "dateStreamStart", json_string(""));
	json_object_set(body, "additionalInfo1", json_string(""));
	json_object_set(body, "additionalInfo2", json_string(""));

	string message = json_dumps(root, 0);

	m_pStompClient->Send(m_stompInfo.serverAppDest.c_str(), tid, message, App()->GetLocalLanguage().toStdString());
}

void LimeCommunicator::OnResponseParseFailed()
{
	MainWindow::Get()->AppClose(AppString::GetString("error_message/error_lime_parse").c_str());
}

void LimeCommunicator::OnCreateChatting(json_t* jsonData, bool success)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::OnCreateChatting] success(%d)", success);

	if (!success)
	{
		MainWindow::Get()->OnCreateChatting(m_chattingInfo, success);
		return;
	}

	if (!json_is_object(jsonData))
	{
		OnResponseParseFailed();
		MainWindow::Get()->OnCreateChatting(m_chattingInfo, false);
		return;
	}
	
	ChattingInfo& info = m_chattingInfo;
	info.groupId = json_get_string_value(jsonData, "groupId");
	info.channelId = json_get_string_value(jsonData, "channelId");
	info.groupType = json_get_string_value(jsonData, "groupType");
	info.name = json_get_string_value(jsonData, "name");
	info.memberCount = json_get_integer_value(jsonData, "memberCount");
	info.profileType = json_get_string_value(jsonData, "profileType");
	info.gameCode = json_get_string_value(jsonData, "gameCode");
	info.groupImageUrl = json_get_string_value(jsonData, "groupImageUrl");
	info.maxMemberCount = json_get_integer_value(jsonData, "maxMemberCount");
	info.dateCreated = json_get_integer_value(jsonData, "dateCreated");
	info.pushStatus = json_get_bool_value(jsonData, "pushStatus");
	info.invitableAll = json_get_bool_value(jsonData, "invitableAll");

	json_t* groupUserInfo = json_object_get(jsonData, "groupUserInfo");
	if (json_is_object(groupUserInfo))
	{
		info.groupUserInfo.groupUserId = json_get_string_value(groupUserInfo, "groupUserId");
		info.groupUserInfo.gameUserId = json_get_string_value(groupUserInfo, "gameUserId");
		info.groupUserInfo.profileImageUrl = json_get_string_value(groupUserInfo, "profileImageUrl");
		info.groupUserInfo.characterId = json_get_string_value(groupUserInfo, "characterId");
		info.groupUserInfo.characterName = json_get_string_value(groupUserInfo, "characterName");
		info.groupUserInfo.serverId = json_get_string_value(groupUserInfo, "rooserverIdmId");
		info.groupUserInfo.serverName = json_get_string_value(groupUserInfo, "serverName");
		info.groupUserInfo.gameCode = json_get_string_value(groupUserInfo, "gameCode");
		info.groupUserInfo.groupUserRole = json_get_string_value(groupUserInfo, "groupUserRole");	
	}

	json_t* ownerInfo = json_object_get(jsonData, "ownerInfo");
	if (json_is_object(groupUserInfo))
	{
		info.ownerInfo.groupUserId = json_get_string_value(groupUserInfo, "groupUserId");
		info.ownerInfo.profileImageUrl = json_get_string_value(groupUserInfo, "profileImageUrl");
		info.ownerInfo.characterName = json_get_string_value(groupUserInfo, "characterName");
	}
	
	MainWindow::Get()->OnCreateChatting(info, success);
}

void LimeCommunicator::OnGetInvitationUserList(json_t* jsonData, bool success)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::OnGetGuildUserList]");

	if (!success)
	{
		MainWindow::Get()->OnGetGuildUserList(m_guildUserInfoList, success);
		return;
	}

	if (!json_is_object(jsonData))
	{
		OnResponseParseFailed();
		MainWindow::Get()->OnGetGuildUserList(m_guildUserInfoList, false);
		return;
	}

	GuildUserInfoList& info = m_guildUserInfoList;
	info.guildName = json_get_string_value(jsonData, "guildName");
	info.gameCode = json_get_integer_value(jsonData, "gameCode");

	json_t* guildUserInfoList = json_object_get(jsonData, "groupUserInfoList");
	if (json_is_array(guildUserInfoList))
	{
		for (int i=0; i<json_array_size(guildUserInfoList); i++)
		{
			json_t* guildUserInfoItem = json_array_get(guildUserInfoList, i);
			if(json_is_object(guildUserInfoItem))
			{
				GuildUserInfo guildUserInfo;
				guildUserInfo.groupUserId = json_get_string_value(guildUserInfoItem, "groupUserId");
				guildUserInfo.gameUserId = json_get_string_value(guildUserInfoItem, "gameUserId");
				guildUserInfo.gameCode = json_get_string_value(guildUserInfoItem, "gameCode");
				guildUserInfo.characterName = json_get_string_value(guildUserInfoItem, "characterName");
				guildUserInfo.serverName = json_get_string_value(guildUserInfoItem, "serverName");
				guildUserInfo.profileImageUrl = json_get_string_value(guildUserInfoItem, "profileImageUrl");
				guildUserInfo.intro = json_get_string_value(guildUserInfoItem, "intro");
				guildUserInfo.className = json_get_string_value(guildUserInfoItem, "className");
				guildUserInfo.dateLastLogout = json_get_integer_value(guildUserInfoItem, "dateLastLogout");
				guildUserInfo.joined = json_get_bool_value(guildUserInfoItem, "joined");
				guildUserInfo.serverId = json_get_string_value(guildUserInfoItem, "serverId");
				guildUserInfo.characterId = json_get_string_value(guildUserInfoItem, "characterId");
				guildUserInfo.characterKey = json_get_string_value(guildUserInfoItem, "characterKey");
				info.guildUserInfoList.push_back(guildUserInfo);
			}
		}
	}	

	MainWindow::Get()->OnGetGuildUserList(info, success);
}

void LimeCommunicator::OnGetGuildUserList(json_t* jsonData, bool success)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::OnGetGuildUserList]");

	if (!success)
	{
		MainWindow::Get()->OnGetGuildUserList(m_guildUserInfoList, success);
		return;
	}

	if (!json_is_object(jsonData))
	{
		OnResponseParseFailed();
		MainWindow::Get()->OnGetGuildUserList(m_guildUserInfoList, false);
		return;
	}

	GuildUserInfoList& info = m_guildUserInfoList;
	info.guildName = json_get_string_value(jsonData, "guildName");
	info.gameCode = json_get_integer_value(jsonData, "gameCode");

	json_t* guildUserInfoList = json_object_get(jsonData, "guildUserInfoList");
	if (json_is_array(guildUserInfoList))
	{
		for (int i=0; i<json_array_size(guildUserInfoList); i++)
		{
			json_t* guildUserInfoItem = json_array_get(guildUserInfoList, i);
			if(json_is_object(guildUserInfoItem))
			{
				GuildUserInfo guildUserInfo;
				guildUserInfo.groupUserId = json_get_string_value(guildUserInfoItem, "groupUserId");
				guildUserInfo.gameUserId = json_get_string_value(guildUserInfoItem, "gameUserId");
				guildUserInfo.gameCode = json_get_string_value(guildUserInfoItem, "gameCode");
				guildUserInfo.characterName = json_get_string_value(guildUserInfoItem, "characterName");
				guildUserInfo.serverName = json_get_string_value(guildUserInfoItem, "serverName");
				guildUserInfo.profileImageUrl = json_get_string_value(guildUserInfoItem, "profileImageUrl");
				guildUserInfo.intro = json_get_string_value(guildUserInfoItem, "intro");
				guildUserInfo.className = json_get_string_value(guildUserInfoItem, "className");
				guildUserInfo.dateLastLogout = json_get_integer_value(guildUserInfoItem, "dateLastLogout");
				guildUserInfo.joined = json_get_bool_value(guildUserInfoItem, "joined");
				guildUserInfo.serverId = json_get_string_value(guildUserInfoItem, "serverId");
				guildUserInfo.characterId = json_get_string_value(guildUserInfoItem, "characterId");
				guildUserInfo.characterKey = json_get_string_value(guildUserInfoItem, "characterKey");
				info.guildUserInfoList.push_back(guildUserInfo);
			}
		}
	}	

	MainWindow::Get()->OnGetGuildUserList(info, success);
}

void LimeCommunicator::OnGetShareTargetList(json_t* jsonData, bool success)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::OnGetShareTargetList]");

	if (!success)
	{
		MainWindow::Get()->OnGetShareTargetList(m_groupInfoList, success);
		return;
	}

	m_groupInfoList.clear();

	if (!json_is_object(jsonData))
	{
		OnResponseParseFailed();
		MainWindow::Get()->OnGetShareTargetList(m_groupInfoList, false);
		return;
	}

	json_t* groupInfoList = json_object_get(jsonData, "groupInfoList");
	if (!json_is_array(groupInfoList))
	{
		OnResponseParseFailed();
		MainWindow::Get()->OnGetShareTargetList(m_groupInfoList, false);
		return;
	}

	for (int i=0; i<json_array_size(groupInfoList); i++)
	{
		json_t* groupInfoItem = json_array_get(groupInfoList, i);
		if(json_is_object(groupInfoItem))
		{
			GroupInfo currentGroup;
			currentGroup.groupId = json_get_string_value(groupInfoItem, "groupId");
			currentGroup.groupType = json_get_string_value(groupInfoItem, "groupType");
			currentGroup.gameChannelType = json_get_string_value(groupInfoItem, "gameChannelType");
			currentGroup.groupImageUrl = json_get_string_value(groupInfoItem, "groupImageUrl");
			currentGroup.name = json_get_string_value(groupInfoItem, "name");

			json_t* channelInfoList = json_object_get(groupInfoItem, "channelInfoList");
			if (json_is_array(channelInfoList))
			{
				for (int i=0; i<json_array_size(channelInfoList); i++)
				{
					json_t* channelInfoItem = json_array_get(channelInfoList, i);
					if(json_is_object(channelInfoItem))
					{
						ChannelInfo currentChannel;
						currentChannel.groupUserId = json_get_string_value(channelInfoItem, "groupUserId");
						currentChannel.groupId = json_get_string_value(channelInfoItem, "groupId");
						currentChannel.channelId = json_get_string_value(channelInfoItem, "channelId");
						currentChannel.channelType = json_get_string_value(channelInfoItem, "channelType");
						currentChannel.name = json_get_string_value(channelInfoItem, "name");
						currentGroup.shareChannelList.push_back(currentChannel);
					}
				}
			}
			m_groupInfoList.push_back(currentGroup);
		}
	}

	MainWindow::Get()->OnGetShareTargetList(m_groupInfoList, success);
}

void LimeCommunicator::OnGetRoomUserList(json_t* jsonData, bool success)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::OnGetRoomUserList]");

	if (!success)
	{
		MainWindow::Get()->OnGetRoomUserList(m_roomUserInfoList, success);
		return;
	}

	if (!json_is_object(jsonData))
	{
		MainWindow::Get()->OnGetRoomUserList(m_roomUserInfoList, false);
		OnResponseParseFailed();
		return;
	}

	m_roomUserInfoList.roomId = json_get_string_value(jsonData, "roomId");
	string cursor = json_get_string_value(jsonData, "cursor");
	m_roomUserInfoList.total_count = json_get_integer_value(jsonData, "count");

	json_t* streamRoomUserInfoList = json_object_get(jsonData, "streamRoomUserInfoList");
	if (!json_is_array(streamRoomUserInfoList))
	{
		MainWindow::Get()->OnGetRoomUserList(m_roomUserInfoList, false);
		OnResponseParseFailed();
		return;
	}

	for (int i=0; i<json_array_size(streamRoomUserInfoList); i++)
	{
		json_t* streamRoomUserItem = json_array_get(streamRoomUserInfoList, i);
		if(json_is_object(streamRoomUserItem))
		{
			StreamRoomUserInfo viewer;
			viewer.gameUserId = json_get_string_value(streamRoomUserItem, "gameUserId");
			viewer.roomUserId = json_get_string_value(streamRoomUserItem, "roomUserId");
			viewer.gameCode = json_get_string_value(streamRoomUserItem, "gameCode");
			viewer.role = json_get_string_value(streamRoomUserItem, "role");
			viewer.name = json_get_string_value(streamRoomUserItem, "name");
			viewer.profileImageSmall = json_get_string_value(streamRoomUserItem, "profileImageSmall");
			viewer.profileImageLarge = json_get_string_value(streamRoomUserItem, "profileImageLarge");
			m_roomUserInfoList.viewerList.push_back(viewer);
		}
	}

	m_roomUserInfoList.loaded_count = m_roomUserInfoList.viewerList.size();

	MainWindow::Get()->OnGetRoomUserList(m_roomUserInfoList, success);
}

void LimeCommunicator::OnGetWatchingUserList(json_t* jsonData, bool success)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::OnGetWatchingUserList]");

	if (!success)
	{
		MainWindow::Get()->OnGetWatchingUserList(m_roomUserInfoList, success);
		return;
	}

	if (!json_is_object(jsonData))
	{
		MainWindow::Get()->OnGetWatchingUserList(m_roomUserInfoList, false);
		OnResponseParseFailed();
		return;
	}

	m_roomUserInfoList.roomId = json_get_string_value(jsonData, "roomId");
	string cursor = json_get_string_value(jsonData, "cursor");
	m_roomUserInfoList.total_count = json_get_integer_value(jsonData, "count");

	json_t* streamRoomUserInfoList = json_object_get(jsonData, "streamRoomUserInfoList");
	if (!json_is_array(streamRoomUserInfoList))
	{
		MainWindow::Get()->OnGetRoomUserList(m_roomUserInfoList, false);
		OnResponseParseFailed();
		return;
	}

	for (int i = 0; i < json_array_size(streamRoomUserInfoList); i++)
	{
		json_t* streamRoomUserItem = json_array_get(streamRoomUserInfoList, i);
		if (json_is_object(streamRoomUserItem))
		{
			StreamRoomUserInfo viewer;
			viewer.gameUserId = json_get_string_value(streamRoomUserItem, "gameUserId");
			viewer.roomUserId = json_get_string_value(streamRoomUserItem, "roomUserId");
			viewer.gameCode = json_get_string_value(streamRoomUserItem, "gameCode");
			viewer.role = json_get_string_value(streamRoomUserItem, "role");
			viewer.name = json_get_string_value(streamRoomUserItem, "name");
			viewer.profileImageSmall = json_get_string_value(streamRoomUserItem, "profileImageSmall");
			viewer.profileImageLarge = json_get_string_value(streamRoomUserItem, "profileImageLarge");
			m_roomUserInfoList.viewerList.push_back(viewer);
		}
	}

	m_roomUserInfoList.loaded_count = m_roomUserInfoList.viewerList.size();

	MainWindow::Get()->OnGetRoomUserList(m_roomUserInfoList, success);
}

void LimeCommunicator::OnDeportRoom(json_t* jsonData, bool success)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::OnDeportRoom]");

	if (!success)
	{
		MainWindow::Get()->OnDeportRoom(success);
		return;
	}

	json_t* viewMessage = json_object_get(jsonData, "viewMessage");
	if (json_is_object(viewMessage))
	{
		string strViewMessage = StringUtils::GetStringFromUtf8(json_get_string_value(viewMessage, "message"));
		LOG(INFO) << format_string("[LimeCommunicator::OnDeportRoom] viewMessage(%s)", strViewMessage.c_str());
	}

	MainWindow::Get()->OnDeportRoom(success);
}

void LimeCommunicator::OnGetUnclosedRoomInfo(json_t* jsonData, bool success)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::OnGetUnclosedRoomInfo]");

	if (!success)
	{
		return;
	}

	if (!json_is_object(jsonData))
	{
		OnResponseParseFailed();
		return;
	}

	json_t* roomInfo = json_object_get(jsonData, "streamRoomInfo");
	if (!json_is_object(roomInfo))
	{
		// 종료되지 않은 방이 없으면 CreateRoom()
		Send_CreateRoom();
		return;
	}

	// parse stream room info 
	m_roomInfo.roomId = json_get_string_value(roomInfo, "roomId");
	m_roomInfo.gameCode = json_get_string_value(roomInfo, "gameCode");
	m_roomInfo.type = json_get_string_value(roomInfo, "type");
	m_roomInfo.name = json_get_string_value(roomInfo, "name");
	m_roomInfo.description = json_get_string_value(roomInfo, "description");
	m_roomInfo.memberCount = json_get_integer_value(roomInfo, "memberCount");
	m_roomInfo.watchingCount = json_get_integer_value(roomInfo, "watchingCount");
	m_roomInfo.viewCount = json_get_integer_value(roomInfo, "viewCount");
	m_roomInfo.dateCreated = json_get_integer_value(roomInfo, "dateCreated");
	m_roomInfo.closed = json_get_bool_value(roomInfo, "closed");
	m_roomInfo.vodPublished = json_get_bool_value(roomInfo, "vodPublished");
	m_roomInfo.topicName = json_get_string_value(roomInfo, "topicName");
	m_roomInfo.streamStatus = json_get_string_value(roomInfo, "streamStatus");
	m_roomInfo.thumbnailUrl = json_get_string_value(roomInfo, "thumbnailUrl");
	m_roomInfo.inviteLinkUrl = json_get_string_value(roomInfo, "inviteLinkUrl");
	m_roomInfo.elapsedSecond = json_get_integer_value(roomInfo, "elapsedSecond");
	m_roomInfo.closeRemainSecond = json_get_integer_value(roomInfo, "closeRemainSecond");
	m_roomInfo.dateReserved = json_get_integer_value(roomInfo, "dateReserved");
	m_roomInfo.dateStreamStart = json_get_integer_value(roomInfo, "dateStreamStart");

	m_roomId = m_roomInfo.roomId;

	// parse stream room owner info 
	StreamRoomUserInfo stOwnerInfo;
	json_t* ownerUserInfo = json_object_get(jsonData, "ownerUserInfo");
	if (json_is_object(ownerUserInfo))
	{
		stOwnerInfo.gameUserId = json_get_string_value(ownerUserInfo, "gameUserId");
		stOwnerInfo.roomUserId = json_get_string_value(ownerUserInfo, "roomUserId");
		stOwnerInfo.gameCode = json_get_string_value(ownerUserInfo, "gameCode");
		stOwnerInfo.role = json_get_string_value(ownerUserInfo, "role");
		stOwnerInfo.name = json_get_string_value(ownerUserInfo, "name");
		stOwnerInfo.profileImageSmall = json_get_string_value(ownerUserInfo, "profileImageSmall");
		stOwnerInfo.profileImageLarge = json_get_string_value(ownerUserInfo, "profileImageLarge");
	}

	// parse stream room user info 
	StreamRoomUserInfo stUserInfo;
	json_t* userInfo = json_object_get(jsonData, "streamRoomUserInfo");
	if (json_is_object(userInfo))
	{
		stUserInfo.gameUserId = json_get_string_value(userInfo, "gameUserId");
		stUserInfo.roomUserId = json_get_string_value(userInfo, "roomUserId");
		stUserInfo.gameCode = json_get_string_value(userInfo, "gameCode");
		stUserInfo.role = json_get_string_value(userInfo, "role");
		stUserInfo.name = json_get_string_value(userInfo, "name");
		stUserInfo.profileImageSmall = json_get_string_value(userInfo, "profileImageSmall");
		stUserInfo.profileImageLarge = json_get_string_value(userInfo, "profileImageLarge");
	}

	if (AppConfig::Instance()->is_rejoin_mediaroom)
	{
		Send_JoinRoom(m_roomInfo.roomId);
	}
	else
	{
		Send_CloseRoom();
		m_roomId = "UnclosedRoom";
	}
}

void LimeCommunicator::OnReconnectRoom(json_t* jsonData, bool success)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::OnCreateRoom]");

	StreamRoomUserInfo stOwnerInfo;
	if (!success)
	{
		SetStreamingState(eStreamingState::eSS_Error);
		MainWindow::Get()->OnReconnectRoom(m_roomInfo, stOwnerInfo, success);
		return;
	}

	if (!json_is_object(jsonData))
	{
		OnResponseParseFailed();
		SetStreamingState(eStreamingState::eSS_Error);
		MainWindow::Get()->OnReconnectRoom(m_roomInfo, stOwnerInfo, false);
		return;
	}

	json_t* roomInfo = json_object_get(jsonData, "streamRoomInfo");
	if (!json_is_object(roomInfo))
	{
		OnResponseParseFailed();
		SetStreamingState(eStreamingState::eSS_Error);
		MainWindow::Get()->OnReconnectRoom(m_roomInfo, stOwnerInfo, false);
		return;
	}

	// parse stream room info 
	m_roomInfo.roomId = json_get_string_value(roomInfo, "roomId");
	m_roomInfo.gameCode = json_get_string_value(roomInfo, "gameCode");
	m_roomInfo.type = json_get_string_value(roomInfo, "type");
	m_roomInfo.name = json_get_string_value(roomInfo, "name");
	m_roomInfo.description = json_get_string_value(roomInfo, "description");
	m_roomInfo.memberCount = json_get_integer_value(roomInfo, "memberCount");
	m_roomInfo.watchingCount = json_get_integer_value(roomInfo, "watchingCount");
	m_roomInfo.viewCount = json_get_integer_value(roomInfo, "viewCount");
	m_roomInfo.dateCreated = json_get_integer_value(roomInfo, "dateCreated");
	m_roomInfo.closed = json_get_bool_value(roomInfo, "closed");
	m_roomInfo.vodPublished = json_get_bool_value(roomInfo, "vodPublished");
	m_roomInfo.topicName = json_get_string_value(roomInfo, "topicName");
	m_roomInfo.streamStatus = json_get_string_value(roomInfo, "streamStatus");
	m_roomInfo.thumbnailUrl = json_get_string_value(roomInfo, "thumbnailUrl");
	m_roomInfo.inviteLinkUrl = json_get_string_value(roomInfo, "inviteLinkUrl");
	m_roomInfo.elapsedSecond = json_get_integer_value(roomInfo, "elapsedSecond");
	m_roomInfo.closeRemainSecond = json_get_integer_value(roomInfo, "closeRemainSecond");
	m_roomInfo.dateReserved = json_get_integer_value(roomInfo, "dateReserved");
	m_roomInfo.dateStreamStart = json_get_integer_value(roomInfo, "dateStreamStart");

	// parse stream room owner info 
	json_t* ownerUserInfo = json_object_get(jsonData, "ownerUserInfo");
	if (json_is_object(ownerUserInfo))
	{
		stOwnerInfo.gameUserId = json_get_string_value(ownerUserInfo, "gameUserId");
		stOwnerInfo.roomUserId = json_get_string_value(ownerUserInfo, "roomUserId");
		stOwnerInfo.gameCode = json_get_string_value(ownerUserInfo, "gameCode");
		stOwnerInfo.role = json_get_string_value(ownerUserInfo, "role");
		stOwnerInfo.name = json_get_string_value(ownerUserInfo, "name");
		stOwnerInfo.profileImageSmall = json_get_string_value(ownerUserInfo, "profileImageSmall");
		stOwnerInfo.profileImageLarge = json_get_string_value(ownerUserInfo, "profileImageLarge");
	}

	MainWindow::Get()->OnReconnectRoom(m_roomInfo, stOwnerInfo, success);
}

void LimeCommunicator::OnCreateRoom(json_t* jsonData, bool success)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::OnCreateRoom]");

	StreamRoomUserInfo stOwnerInfo;
	if (!success)
	{
		SetStreamingState(eStreamingState::eSS_Error);
		MainWindow::Get()->OnCreateRoom(m_roomInfo, stOwnerInfo, success);
		return;
	}

	if (!json_is_object(jsonData))
	{
		OnResponseParseFailed();
		SetStreamingState(eStreamingState::eSS_Error);
		MainWindow::Get()->OnCreateRoom(m_roomInfo, stOwnerInfo, false);
		return;
	}

	json_t* roomInfo = json_object_get(jsonData, "streamRoomInfo");
	if (!json_is_object(roomInfo))
	{
		OnResponseParseFailed();
		SetStreamingState(eStreamingState::eSS_Error);
		MainWindow::Get()->OnCreateRoom(m_roomInfo, stOwnerInfo, false);
		return;
	}

	// parse stream room info 
	m_roomInfo.roomId = json_get_string_value(roomInfo, "roomId");
	m_roomInfo.gameCode = json_get_string_value(roomInfo, "gameCode");
	m_roomInfo.type = json_get_string_value(roomInfo, "type");
	m_roomInfo.name = json_get_string_value(roomInfo, "name");
	m_roomInfo.description = json_get_string_value(roomInfo, "description");
	m_roomInfo.memberCount = json_get_integer_value(roomInfo, "memberCount");
	m_roomInfo.watchingCount = json_get_integer_value(roomInfo, "watchingCount");
	m_roomInfo.viewCount = json_get_integer_value(roomInfo, "viewCount");
	m_roomInfo.dateCreated = json_get_integer_value(roomInfo, "dateCreated");
	m_roomInfo.closed = json_get_bool_value(roomInfo, "closed");
	m_roomInfo.vodPublished = json_get_bool_value(roomInfo, "vodPublished");
	m_roomInfo.topicName = json_get_string_value(roomInfo, "topicName");
	m_roomInfo.streamStatus = json_get_string_value(roomInfo, "streamStatus");
	m_roomInfo.thumbnailUrl = json_get_string_value(roomInfo, "thumbnailUrl");
	m_roomInfo.inviteLinkUrl = json_get_string_value(roomInfo, "inviteLinkUrl");
	m_roomInfo.elapsedSecond = json_get_integer_value(roomInfo, "elapsedSecond");
	m_roomInfo.closeRemainSecond = json_get_integer_value(roomInfo, "closeRemainSecond");
	m_roomInfo.dateReserved = json_get_integer_value(roomInfo, "dateReserved");
	m_roomInfo.dateStreamStart = json_get_integer_value(roomInfo, "dateStreamStart");

	// parse stream room owner info 
	json_t* ownerUserInfo = json_object_get(jsonData, "ownerUserInfo");
	if (json_is_object(ownerUserInfo))
	{
		stOwnerInfo.gameUserId = json_get_string_value(ownerUserInfo, "gameUserId");
		stOwnerInfo.roomUserId = json_get_string_value(ownerUserInfo, "roomUserId");
		stOwnerInfo.gameCode = json_get_string_value(ownerUserInfo, "gameCode");
		stOwnerInfo.role = json_get_string_value(ownerUserInfo, "role");
		stOwnerInfo.name = json_get_string_value(ownerUserInfo, "name");
		stOwnerInfo.profileImageSmall = json_get_string_value(ownerUserInfo, "profileImageSmall");
		stOwnerInfo.profileImageLarge = json_get_string_value(ownerUserInfo, "profileImageLarge");
	}

	// parse stream room user info 
	StreamRoomUserInfo stUserInfo;
	json_t* userInfo = json_object_get(jsonData, "streamRoomUserInfo");
	if (json_is_object(userInfo))
	{
		stUserInfo.gameUserId = json_get_string_value(userInfo, "gameUserId");
		stUserInfo.roomUserId = json_get_string_value(userInfo, "roomUserId");
		stUserInfo.gameCode = json_get_string_value(userInfo, "gameCode");
		stUserInfo.role = json_get_string_value(userInfo, "role");
		stUserInfo.name = json_get_string_value(userInfo, "name");
		stUserInfo.profileImageSmall = json_get_string_value(userInfo, "profileImageSmall");
		stUserInfo.profileImageLarge = json_get_string_value(userInfo, "profileImageLarge");
	}

	MainWindow::Get()->OnCreateRoom(m_roomInfo, stOwnerInfo, success);
	Send_JoinRoom(m_roomInfo.roomId);
}

void LimeCommunicator::OnJoinRoom(json_t* jsonData, bool success)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::OnJoinRoom]");

	if (!success)
	{
		SetStreamingState(eStreamingState::eSS_Error);
		MainWindow::Get()->OnJoinRoom(success);
		return;
	}

	if (!json_is_object(jsonData))
	{
		OnResponseParseFailed();
		SetStreamingState(eStreamingState::eSS_Error);
		MainWindow::Get()->OnJoinRoom(false);
		return;
	}

	json_t* roomInfo = json_object_get(jsonData, "streamRoomInfo");
	if (!json_is_object(roomInfo))
	{
		OnResponseParseFailed();
		SetStreamingState(eStreamingState::eSS_Error);
		MainWindow::Get()->OnJoinRoom(false);
		return;
	}

	// parse stream room info 
	m_roomInfo.roomId = json_get_string_value(roomInfo, "roomId");
	m_roomInfo.gameCode = json_get_string_value(roomInfo, "gameCode");
	m_roomInfo.type = json_get_string_value(roomInfo, "type");
	m_roomInfo.name = json_get_string_value(roomInfo, "name");
	m_roomInfo.description = json_get_string_value(roomInfo, "description");
	m_roomInfo.memberCount = json_get_integer_value(roomInfo, "memberCount");
	m_roomInfo.watchingCount = json_get_integer_value(roomInfo, "watchingCount");
	m_roomInfo.viewCount = json_get_integer_value(roomInfo, "viewCount");
	m_roomInfo.dateCreated = json_get_integer_value(roomInfo, "dateCreated");
	m_roomInfo.closed = json_get_bool_value(roomInfo, "closed");
	m_roomInfo.vodPublished = json_get_bool_value(roomInfo, "vodPublished");
	m_roomInfo.topicName = json_get_string_value(roomInfo, "topicName");
	m_roomInfo.streamStatus = json_get_string_value(roomInfo, "streamStatus");
	m_roomInfo.thumbnailUrl = json_get_string_value(roomInfo, "thumbnailUrl");
	m_roomInfo.inviteLinkUrl = json_get_string_value(roomInfo, "inviteLinkUrl");
	m_roomInfo.elapsedSecond = json_get_integer_value(roomInfo, "elapsedSecond");
	m_roomInfo.closeRemainSecond = json_get_integer_value(roomInfo, "closeRemainSecond");
	m_roomInfo.dateReserved = json_get_integer_value(roomInfo, "dateReserved");
	m_roomInfo.dateStreamStart = json_get_integer_value(roomInfo, "dateStreamStart");

	// backup room info
	m_roomId = m_roomInfo.roomId;
	m_roomName = m_roomInfo.name;

	// parse stream room owner info 
	StreamRoomUserInfo stOwnerInfo;
	json_t* ownerUserInfo = json_object_get(jsonData, "ownerUserInfo");
	if (json_is_object(ownerUserInfo))
	{
		stOwnerInfo.gameUserId = json_get_string_value(ownerUserInfo, "gameUserId");
		stOwnerInfo.roomUserId = json_get_string_value(ownerUserInfo, "roomUserId");
		stOwnerInfo.gameCode = json_get_string_value(ownerUserInfo, "gameCode");
		stOwnerInfo.role = json_get_string_value(ownerUserInfo, "role");
		stOwnerInfo.name = json_get_string_value(ownerUserInfo, "name");
		stOwnerInfo.profileImageSmall = json_get_string_value(ownerUserInfo, "profileImageSmall");
		stOwnerInfo.profileImageLarge = json_get_string_value(ownerUserInfo, "profileImageLarge");
	}

	// parse stream room user info 
	StreamRoomUserInfo stUserInfo;
	json_t* userInfo = json_object_get(jsonData, "streamRoomUserInfo");
	if (json_is_object(userInfo))
	{
		stUserInfo.gameUserId = json_get_string_value(userInfo, "gameUserId");
		stUserInfo.roomUserId = json_get_string_value(userInfo, "roomUserId");
		stUserInfo.gameCode = json_get_string_value(userInfo, "gameCode");
		stUserInfo.role = json_get_string_value(userInfo, "role");
		stUserInfo.name = json_get_string_value(userInfo, "name");
		stUserInfo.profileImageSmall = json_get_string_value(userInfo, "profileImageSmall");
		stUserInfo.profileImageLarge = json_get_string_value(userInfo, "profileImageLarge");
	}

	Send_StartStream(m_roomInfo.roomId);
}

void LimeCommunicator::OnStartStream(json_t* jsonData, bool success)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::OnStartStream]");

	if (!success)
	{
		SetStreamingState(eStreamingState::eSS_Error);
		MainWindow::Get()->OnStartStream(MediaSignalServerInfo{}, "", success);
		return;
	}

	if (!json_is_object(jsonData))
	{
		OnResponseParseFailed();
		SetStreamingState(eStreamingState::eSS_Error);
		MainWindow::Get()->OnStartStream(MediaSignalServerInfo{}, "", false);
		return;
	}

	string strTid = json_get_string_value(jsonData, "tid");
	string strAction = json_get_string_value(jsonData, "action");
	m_authKey = json_get_string_value(jsonData, "authKey");
//	int strKeepAliveInterval = json_get_integer_value(jsonData, "keepAliveInterval");

	json_t* player = json_object_get(jsonData, "player");
	if (!json_is_object(player))
	{
		OnResponseParseFailed();
		SetStreamingState(eStreamingState::eSS_Error);
		MainWindow::Get()->OnStartStream(MediaSignalServerInfo{}, "", false);
		return;
	}

	// parse stream room info 
	PlayerInfo stPlayerInfo;

	stPlayerInfo.serviceType = json_get_string_value(player, "serviceType");
	stPlayerInfo.appGroupCode = json_get_string_value(player, "appGroupCode");
	stPlayerInfo.scope = json_get_string_value(player, "scope");
	stPlayerInfo.roomId = json_get_string_value(player, "roomId");
	stPlayerInfo.playerId = json_get_string_value(player, "playerId");
	stPlayerInfo.playerName = json_get_string_value(player, "playerName");
	stPlayerInfo.role = json_get_string_value(player, "role");

	m_roomId = stPlayerInfo.roomId;

	json_t* mediaRole = json_object_get(player, "mediaRole");
	if (!json_is_object(mediaRole))
	{
		OnResponseParseFailed();
		SetStreamingState(eStreamingState::eSS_Error);
		MainWindow::Get()->OnStartStream(MediaSignalServerInfo{}, "", false);
		return;
	}

	stPlayerInfo.mediaRole.audio = json_get_string_value(mediaRole, "audio");
	stPlayerInfo.mediaRole.video = json_get_string_value(mediaRole, "video");

	json_t* mediaStatus = json_object_get(player, "mediaStatus");
	if (!json_is_object(mediaStatus))
	{
		OnResponseParseFailed();
		SetStreamingState(eStreamingState::eSS_Error);
		MainWindow::Get()->OnStartStream(MediaSignalServerInfo{}, "", false);
		return;
	}

	stPlayerInfo.mediaStatus.audio = json_get_string_value(mediaStatus, "audio");
	stPlayerInfo.mediaStatus.video = json_get_string_value(mediaStatus, "video");

	stPlayerInfo.createdTime = json_get_integer_value(player, "createdTime");

	json_t* mediaSignalServer = json_object_get(jsonData, "mediaSignalServer");
	if (!json_is_object(mediaSignalServer))
	{
		OnResponseParseFailed();
		SetStreamingState(eStreamingState::eSS_Error);
		MainWindow::Get()->OnStartStream(MediaSignalServerInfo{}, "", false);
		return;
	}

	MediaSignalServerInfo signalServerInfo;
	signalServerInfo.httpHost = json_get_string_value(mediaSignalServer, "httpHost");
	signalServerInfo.webSocketUrl = json_get_string_value(mediaSignalServer, "webSocketUrl");

	MainWindow::Get()->OnStartStream(signalServerInfo, m_authKey.c_str(), success);
	SetStreamingState(eStreamingState::eSS_Streaming);
}

void LimeCommunicator::OnStopStream(json_t* jsonData, bool success)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::OnStopStream]");

	UNREFERENCED_PARAMETER(jsonData);
	MainWindow::Get()->OnStopStream(success);
}

void LimeCommunicator::OnPauseStream(json_t* jsonData, bool success)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::OnPauseStream]");

	UNREFERENCED_PARAMETER(jsonData);
}

void LimeCommunicator::OnCloseRoom(json_t* jsonData, bool success)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::OnCloseRoom]");

	UNREFERENCED_PARAMETER(jsonData);
	if (m_roomId == "UnclosedRoom")
	{
		// 기존 방을 종료하고 새로 방송을 시작한다
		Send_CreateRoom();
	}
	else
	{
		MainWindow::Get()->OnCloseRoom(success);
	}
	SetStreamingState(eStreamingState::eSS_Stopped);
}

void LimeCommunicator::OnUpdateRoom(json_t* jsonData, bool success)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::OnUpdateRoom]");

	if (!success)
	{
		MainWindow::Get()->OnUpdateRoom(m_roomInfo, success);
		return;
	}

	if (!json_is_object(jsonData))
	{
		OnResponseParseFailed();
		MainWindow::Get()->OnUpdateRoom(m_roomInfo, false);
		return;
	}

	json_t* roomInfo = json_object_get(jsonData, "streamRoomInfo");
	if (!json_is_object(roomInfo))
	{
		OnResponseParseFailed();
		MainWindow::Get()->OnUpdateRoom(m_roomInfo, false);
		return;
	}

	// parse stream room info 
	m_roomInfo.roomId = json_get_string_value(roomInfo, "roomId");
	m_roomInfo.gameCode = json_get_string_value(roomInfo, "gameCode");
	m_roomInfo.type = json_get_string_value(roomInfo, "type");
	m_roomInfo.name = json_get_string_value(roomInfo, "name");
	m_roomInfo.description = json_get_string_value(roomInfo, "description");
	m_roomInfo.memberCount = json_get_integer_value(roomInfo, "memberCount");
	m_roomInfo.watchingCount = json_get_integer_value(roomInfo, "watchingCount");
	m_roomInfo.viewCount = json_get_integer_value(roomInfo, "viewCount");
	m_roomInfo.dateCreated = json_get_integer_value(roomInfo, "dateCreated");
	m_roomInfo.closed = json_get_bool_value(roomInfo, "closed");
	m_roomInfo.vodPublished = json_get_bool_value(roomInfo, "vodPublished");
	m_roomInfo.topicName = json_get_string_value(roomInfo, "topicName");
	m_roomInfo.streamStatus = json_get_string_value(roomInfo, "streamStatus");
	m_roomInfo.thumbnailUrl = json_get_string_value(roomInfo, "thumbnailUrl");
	m_roomInfo.inviteLinkUrl = json_get_string_value(roomInfo, "inviteLinkUrl");
	m_roomInfo.elapsedSecond = json_get_integer_value(roomInfo, "elapsedSecond");
	m_roomInfo.closeRemainSecond = json_get_integer_value(roomInfo, "closeRemainSecond");
	m_roomInfo.dateReserved = json_get_integer_value(roomInfo, "dateReserved");
	m_roomInfo.dateStreamStart = json_get_integer_value(roomInfo, "dateStreamStart");

	// backup room info
	m_roomId = m_roomInfo.roomId;
	m_roomName = m_roomInfo.name;

	// parse stream room owner info 
	StreamRoomUserInfo stOwnerInfo;
	json_t* ownerUserInfo = json_object_get(jsonData, "ownerUserInfo");
	if (json_is_object(ownerUserInfo))
	{
		stOwnerInfo.gameUserId = json_get_string_value(ownerUserInfo, "gameUserId");
		stOwnerInfo.roomUserId = json_get_string_value(ownerUserInfo, "roomUserId");
		stOwnerInfo.gameCode = json_get_string_value(ownerUserInfo, "gameCode");
		stOwnerInfo.role = json_get_string_value(ownerUserInfo, "role");
		stOwnerInfo.name = json_get_string_value(ownerUserInfo, "name");
		stOwnerInfo.profileImageSmall = json_get_string_value(ownerUserInfo, "profileImageSmall");
		stOwnerInfo.profileImageLarge = json_get_string_value(ownerUserInfo, "profileImageLarge");
	}

	MainWindow::Get()->OnUpdateRoom(m_roomInfo, success);
}

void LimeCommunicator::OnEventJoin(json_t* jsonData)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::OnEventJoin]");

	if (!json_is_object(jsonData))
	{
		OnResponseParseFailed();
		return;
	}

	json_t* roomUserInfo = json_object_get(jsonData, "streamRoomUserInfo");
	if (!json_is_object(roomUserInfo))
	{
		OnResponseParseFailed();
		return;
	}

	json_t* roomUpdateStatus = json_object_get(jsonData, "streamRoomUpdateStatusInfo");
	if (!json_is_object(roomUserInfo))
	{
		OnResponseParseFailed();
		return;
	}

	m_roomUserInfo.roomUserId = json_get_string_value(roomUserInfo, "roomId");
	m_roomUserInfo.gameUserId = json_get_string_value(roomUserInfo, "gameUserId");
	m_roomUserInfo.gameCode = json_get_string_value(roomUserInfo, "gameCode");
	m_roomUserInfo.role = json_get_string_value(roomUserInfo, "role");
	m_roomUserInfo.name = json_get_string_value(roomUserInfo, "name");
	m_roomUserInfo.intro = json_get_string_value(roomUserInfo, "intro");
	m_roomUserInfo.profileImageSmall = json_get_string_value(roomUserInfo, "profileImageSmall");
	m_roomUserInfo.profileImageLarge = json_get_string_value(roomUserInfo, "profileImageLarge");
	m_roomUserInfo.userThemeColor = json_get_string_value(roomUserInfo, "userThemeColor");
	m_roomUserInfo.lastMessage = json_get_string_value(roomUserInfo, "lastMessage");
	m_roomUserInfo.banned = json_get_bool_value(roomUserInfo, "banned");
	m_roomUserInfo.deported = json_get_bool_value(roomUserInfo, "deported");
	m_roomUserInfo.hide = json_get_bool_value(roomUserInfo, "hide");
	m_roomUserInfo.serverName = json_get_string_value(roomUserInfo, "serverName");
	m_roomUserInfo.dateRestrictChatUntil = json_get_integer_value(roomUserInfo, "dateRestrictChatUntil");

	m_roomUpdateStatusInfo.streamStatus = json_get_string_value(roomUpdateStatus,"streamStatus");
	m_roomUpdateStatusInfo.memberCount = json_get_integer_value(roomUpdateStatus, "memberCount");
	m_roomUpdateStatusInfo.watchingCount = json_get_integer_value(roomUpdateStatus, "watchingCount");
	m_roomUpdateStatusInfo.viewCount = json_get_integer_value(roomUpdateStatus, "viewCount");

	LOG(INFO) << format_string("[LimeCommunicator::OnEventJoin] gameUserId(%s), name(%s)", m_roomUserInfo.gameUserId.c_str(), StringUtils::GetStringFromUtf8(m_roomUserInfo.name).c_str());

	MainWindow::Get()->OnEventJoin(m_roomUserInfo, m_roomUpdateStatusInfo);
}

void LimeCommunicator::OnEventLeave(json_t* jsonData)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::OnEventLeave]");

	if (!json_is_object(jsonData))
	{
		OnResponseParseFailed();
		return;
	}

	json_t* roomUserInfo = json_object_get(jsonData, "streamRoomUserInfo");
	if (!json_is_object(roomUserInfo))
	{
		OnResponseParseFailed();
		return;
	}

	json_t* roomUpdateStatus = json_object_get(jsonData, "streamRoomUpdateStatusInfo");
	if (!json_is_object(roomUserInfo))
	{
		OnResponseParseFailed();
		return;
	}

	m_roomUserInfo.roomUserId = json_get_string_value(roomUserInfo, "roomId");
	m_roomUserInfo.gameUserId = json_get_string_value(roomUserInfo, "gameUserId");
	m_roomUserInfo.gameCode = json_get_string_value(roomUserInfo, "gameCode");
	m_roomUserInfo.role = json_get_string_value(roomUserInfo, "role");
	m_roomUserInfo.name = json_get_string_value(roomUserInfo, "name");
	m_roomUserInfo.intro = json_get_string_value(roomUserInfo, "intro");
	m_roomUserInfo.profileImageSmall = json_get_string_value(roomUserInfo, "profileImageSmall");
	m_roomUserInfo.profileImageLarge = json_get_string_value(roomUserInfo, "profileImageLarge");
	m_roomUserInfo.userThemeColor = json_get_string_value(roomUserInfo, "userThemeColor");
	m_roomUserInfo.lastMessage = json_get_string_value(roomUserInfo, "lastMessage");
	m_roomUserInfo.banned = json_get_bool_value(roomUserInfo, "banned");
	m_roomUserInfo.deported = json_get_bool_value(roomUserInfo, "deported");
	m_roomUserInfo.hide = json_get_bool_value(roomUserInfo, "hide");
	m_roomUserInfo.serverName = json_get_string_value(roomUserInfo, "serverName");
	m_roomUserInfo.dateRestrictChatUntil = json_get_integer_value(roomUserInfo, "dateRestrictChatUntil");

	m_roomUpdateStatusInfo.streamStatus = json_get_string_value(roomUpdateStatus, "streamStatus");
	m_roomUpdateStatusInfo.memberCount = json_get_integer_value(roomUpdateStatus, "memberCount");
	m_roomUpdateStatusInfo.watchingCount = json_get_integer_value(roomUpdateStatus, "watchingCount");
	m_roomUpdateStatusInfo.viewCount = json_get_integer_value(roomUpdateStatus, "viewCount");

	LOG(INFO) << format_string("[LimeCommunicator::OnEventLeave] gameUserId(%s), name(%s)", m_roomUserInfo.gameUserId.c_str(), StringUtils::GetStringFromUtf8(m_roomUserInfo.name).c_str());

	MainWindow::Get()->OnEventLeave(m_roomUserInfo, m_roomUpdateStatusInfo);
}

void LimeCommunicator::OnEventDeport(json_t* jsonData)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::OnEventDeport]");

	if (!json_is_object(jsonData))
	{
		OnResponseParseFailed();
		return;
	}

	json_t* roomUserInfo = json_object_get(jsonData, "streamRoomUserInfo");
	if (!json_is_object(roomUserInfo))
	{
		OnResponseParseFailed();
		return;
	}

	json_t* roomUpdateStatus = json_object_get(jsonData, "streamRoomUpdateStatusInfo");
	if (!json_is_object(roomUserInfo))
	{
		OnResponseParseFailed();
		return;
	}

	json_t* viewMessage = json_object_get(jsonData, "viewMessage");
	if (json_is_object(viewMessage))
	{
		string strViewMessage = StringUtils::GetStringFromUtf8(json_get_string_value(viewMessage, "message"));
		LOG(INFO) << format_string("[LimeCommunicator::OnEventDeport] viewMessage(%s)", strViewMessage.c_str());
	}

	m_roomUserInfo.roomUserId = json_get_string_value(roomUserInfo, "roomId");
	m_roomUserInfo.gameUserId = json_get_string_value(roomUserInfo, "gameUserId");
	m_roomUserInfo.gameCode = json_get_string_value(roomUserInfo, "gameCode");
	m_roomUserInfo.role = json_get_string_value(roomUserInfo, "role");
	m_roomUserInfo.name = json_get_string_value(roomUserInfo, "name");
	m_roomUserInfo.intro = json_get_string_value(roomUserInfo, "intro");
	m_roomUserInfo.profileImageSmall = json_get_string_value(roomUserInfo, "profileImageSmall");
	m_roomUserInfo.profileImageLarge = json_get_string_value(roomUserInfo, "profileImageLarge");
	m_roomUserInfo.userThemeColor = json_get_string_value(roomUserInfo, "userThemeColor");
	m_roomUserInfo.lastMessage = json_get_string_value(roomUserInfo, "lastMessage");
	m_roomUserInfo.banned = json_get_bool_value(roomUserInfo, "banned");
	m_roomUserInfo.deported = json_get_bool_value(roomUserInfo, "deported");
	m_roomUserInfo.hide = json_get_bool_value(roomUserInfo, "hide");
	m_roomUserInfo.serverName = json_get_string_value(roomUserInfo, "serverName");
	m_roomUserInfo.dateRestrictChatUntil = json_get_integer_value(roomUserInfo, "dateRestrictChatUntil");

	m_roomUpdateStatusInfo.streamStatus = json_get_string_value(roomUpdateStatus, "streamStatus");
	m_roomUpdateStatusInfo.memberCount = json_get_integer_value(roomUpdateStatus, "memberCount");
	m_roomUpdateStatusInfo.watchingCount = json_get_integer_value(roomUpdateStatus, "watchingCount");
	m_roomUpdateStatusInfo.viewCount = json_get_integer_value(roomUpdateStatus, "viewCount");

	LOG(INFO) << format_string("[LimeCommunicator::OnEventDeport] gameUserId(%s), name(%s)", m_roomUserInfo.gameUserId.c_str(), StringUtils::GetStringFromUtf8(m_roomUserInfo.name).c_str());

	MainWindow::Get()->OnEventDeport(m_roomUserInfo, m_roomUpdateStatusInfo);
}

void LimeCommunicator::OnEventCount(json_t* jsonData)
{
	LOG(DEBUG) << format_string("[LimeCommunicator::OnEventCount]");

	if (!json_is_object(jsonData))
	{
		OnResponseParseFailed();
		return;
	}

	json_t* roomInfo = json_object_get(jsonData, "streamRoomUpdateStatusInfo");
	if (!json_is_object(roomInfo))
	{
		OnResponseParseFailed();
		return;
	}

	// parse stream room info 
	StreamRoomUpdateStatusInfo stRoomInfo;

	stRoomInfo.streamStatus = json_get_string_value(roomInfo, "streamStatus");
	stRoomInfo.memberCount = json_get_integer_value(roomInfo, "memberCount");
	stRoomInfo.watchingCount = json_get_integer_value(roomInfo, "watchingCount");
	stRoomInfo.viewCount = json_get_integer_value(roomInfo, "viewCount");

	m_roomInfo.memberCount = stRoomInfo.memberCount;
	m_roomInfo.watchingCount = stRoomInfo.watchingCount;
	m_roomInfo.viewCount = stRoomInfo.viewCount;
	
	LOG(INFO) << format_string("[LimeCommunicator::OnEventCount] memberCount(%d), watchingCount(%d), viewCount(%d)", m_roomInfo.memberCount, m_roomInfo.watchingCount, m_roomInfo.viewCount);

	MainWindow::Get()->OnEventCount(m_roomInfo);
}

void LimeCommunicator::OnEventChatGroupLeave(json_t* jsonData)
{
	LOG(INFO) << format_string("[LimeCommunicator::OnEventChatGroupLeave]");
	MainWindow::Get()->OnEventChatGroupLeave();
}

void LimeCommunicator::OnEventStopStream(json_t* jsonData, bool isForceQuit)
{
	LOG(INFO) << format_string("[LimeCommunicator::OnEventStopStream] isForceQuit(%d)", isForceQuit);
	MainWindow::Get()->OnEventStopStream(isForceQuit);
}