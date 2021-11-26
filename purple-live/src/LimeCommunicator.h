#pragma once

#include "Singleton.h"
#include "QTWebStompClient.h"
#include "lime-defines.h"
#include "jansson.h"
#include "RetryConnectionHelper.h"

class LimeEventHandler
{
public:
	virtual void OnPubServerConnected(bool reconnect) = 0;
	virtual void OnChangeStreamingState(eStreamingState streamingState) = 0;
	virtual void OnCreateChatting(ChattingInfo& info, bool success) = 0;
	virtual void OnGetGuildUserList(GuildUserInfoList& info, bool success) = 0;
	virtual void OnGetShareTargetList(vector<GroupInfo>& groupInfoList, bool success) = 0;
	virtual void OnGetRoomUserList(StreamRoomUserInfoList& viewerList, bool success) = 0;
	virtual void OnGetWatchingUserList(StreamRoomUserInfoList& viewerList, bool success) = 0;
	virtual void OnDeportRoom(bool success) = 0;
	virtual void OnReconnectRoom(StreamRoomInfo& roomInfo, StreamRoomUserInfo& ownerInfo, bool success) = 0;
	virtual void OnCreateRoom(StreamRoomInfo& roomInfo, StreamRoomUserInfo& ownerInfo, bool success) = 0;
	virtual void OnJoinRoom(bool success) = 0;
	virtual void OnUpdateRoom(StreamRoomInfo& streamRoomInfo, bool success) = 0;
	virtual void OnStartStream(MediaSignalServerInfo& mediaSignalServerInfo, const char* strAuthKey, bool success) = 0;
	virtual void OnStopStream(bool success) = 0;
	virtual void OnCloseRoom(bool success) = 0;
	virtual void OnError() = 0;
	virtual void OnEventCount(StreamRoomInfo& streamRoomInfo) = 0;
	virtual void OnEventChatGroupLeave() = 0;
	virtual void OnEventJoin(StreamRoomUserInfo& streamRoomUserInfo, StreamRoomUpdateStatusInfo& streamRoomUpdateStatusInfo) = 0;
	virtual void OnEventLeave(StreamRoomUserInfo& streamRoomUserInfo, StreamRoomUpdateStatusInfo& streamRoomUpdateStatusInfo) = 0;
	virtual void OnEventDeport(StreamRoomUserInfo& streamRoomUserInfo, StreamRoomUpdateStatusInfo& streamRoomUpdateStatusInfo) = 0;
	virtual void OnEventStopStream(bool isForceQuit) = 0;
};

class LimeCommunicator : public StompReciever, public CommunityLiveCore::Singleton<LimeCommunicator>
{
public:
	explicit LimeCommunicator();
	virtual ~LimeCommunicator();	

	void				SetForceExcute(bool enable);
	void				SetRoomID(const char* name);
	void				SetRoomName(const char* name);
	void				SetShareTarget(const char* groupUserId, const char* channelId);
	void				SetFirstShareTarget();

	int					GetCurrentCharacterIndex();
	void				SetCurrentCharacterIndex(int index);
	int					GetCurrentCharacterCount();
	std::string			GetAuthKey();
	bool				GetGameUserInfo(int index, GameUserInfo& game_user_info);
	int					GetCharacterIndex(const char* characterName);
	bool				GetCanStreamfromIndex(int index);
	std::string			GetCharacterNamefromIndex(int index);
	std::string			GetGamecodefromIndex(int index);
	std::string			GetGuildNamefromIndex(int index);
	std::string			GetCharacterImageUrlfromIndex(int index);
	std::string			GetCharacterServerNamefromIndex(int index);
	std::string			GetFirstGameUserID();
	std::string			GetGameUserID(const char* characterName);
	std::string			GetUserImageFromCharacterName(const char* characterName);
	std::string			GetCreateChattingParams(const char* gameUserID, const GuildUserInfoList& info);
	std::string			GetInviteLinkUrl();
	
	bool				GetCharactorList(const char* gameCode, const char* token, eServiceNetwork sn, const char* appVersion);
	bool				CreateChatting(const char* gameUserID, const char* token, const char* deviceId, eServiceNetwork sn, const char* appVersion, const char* params);
	ChattingInfo&		GetCreateChatInfo();
	bool				GetGuildUserList(const char* gameUserID, const char* token, const char* deviceId, eServiceNetwork sn, const char* appVersion);
	GuildUserInfoList&	GetGuildUserList();
	bool				GetShareTargetList(const char* gameUserID, const char* token, const char* deviceId, eServiceNetwork sn, const char* appVersion);

	void				CheckRetryConnection();
	void				StopRetryConnection();
	void				ReconnectLiveWebServer();
	void				ConnectLiveWebServer();
	
	eRetryConnectionState	GetRetryConnectionState();
	eConnectionState	GetConnectionState();
	eStreamingState		GetStreamingState();
	void				SetStreamingState(eStreamingState state);
	void				ConnectedProcess(bool reconnect = false);

	bool				ConnectPubServer(const char* gameUserID, const char* token, const char* deviceId, eServiceNetwork sn, const char* appVersion);
	void				RequestStartStreaming();
	void				RequestStopStreaming();
	void				RequestCloseRoom();

	void				RequestCreateChatting(const char* params);
	void				RequestGuildUserList();
	void				RequestShareTargetList();
	bool				RequestRoomUserList(bool isAppend=false, int count=100);
	bool				RequestWatchingUserList(bool isAppend = false, int count = 100);
	void				RequestDeportRoom(const DeportUserInfo& info);
	
	vector<GroupInfo>&		GetGroupInfo() { return m_groupInfoList; }

	
	bool				IsLogin();
	bool				Relogin();
	bool				Login(const char* gameUserID, const char* token, const char* deviceId, eServiceNetwork sn, const char* appVersion);
	void				Logout();
	bool				GetSubscriptionInfo();
	void				ClearLimeToken();
	std::string			GetLimeToken(const char* gameUserID, const char* token, const char* deviceId, eServiceNetwork sn, const char* appVersion);
	
	
protected:

	void				SetRetryConnectionState(eRetryConnectionState state);

	// stomp receiver
	virtual void OnConnectedWebSocket() override;
	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnMessage(const StompMessage &s) override;
	virtual void OnErrorMessage(const char* message) override;
	virtual void OnError(QAbstractSocket::SocketError error) override;

	// stomp send/receive msg
	void Send_CreateChatting(const char* params);
	void Send_GetGuildUserList();
	void Send_GetShareTargetList();
	void Send_GetRoomUserList(string roomId, int cursor, int count = 100/*(default 100, MAX 100)*/);
	void Send_GetWatchingUserList(string roomId, int cursor, int count = 100/*(default 100, MAX 100)*/);
	void Send_DeportRoom(string roomId, const DeportUserInfo& info);
	void Send_GetUnclosedRoomInfo();
	void Send_ReconnectRoom();
	void Send_CreateRoom();
	void Send_JoinRoom(string roomId);
	void Send_StartStream(string roomId);
	void Send_StopStream();
	void Send_PauseStream();
	void Send_CloseRoom();
	void Send_UpdateRoom(const string roomName);

	void OnResponseParseFailed();
	void OnCreateChatting(json_t* jsonData, bool success = true);
	void OnGetInvitationUserList(json_t* jsonData, bool success = true);
	void OnGetGuildUserList(json_t* jsonData, bool success = true);
	void OnGetShareTargetList(json_t* jsonData, bool success = true);
	void OnGetRoomUserList(json_t* jsonData, bool success = true);
	void OnGetWatchingUserList(json_t* jsonData, bool success = true);
	void OnDeportRoom(json_t* jsonData, bool success = true);
	void OnGetUnclosedRoomInfo(json_t* jsonData, bool success = true);
	void OnReconnectRoom(json_t* jsonData, bool success = true);
	void OnCreateRoom(json_t* jsonData, bool success = true);
	void OnJoinRoom(json_t* jsonData, bool success = true);
	void OnStartStream(json_t* jsonData, bool success = true);
	void OnStopStream(json_t* jsonData, bool success = true);
	void OnPauseStream(json_t* jsonData, bool success = true);
	void OnCloseRoom(json_t* jsonData, bool success = true);
	void OnUpdateRoom(json_t* jsonData, bool success = true);

	// EVENT
	void OnEventJoin(json_t* jsonData);
	void OnEventLeave(json_t* jsonData);
	void OnEventDeport(json_t* jsonData);
	void OnEventCount(json_t* jsonData);
	void OnEventChatGroupLeave(json_t* jsonData);
	void OnEventStopStream(json_t* jsonData, bool isForceQuit);

private:
	bool						m_isForceExcute = false;
	int							m_current_charactor_index;
	StompInfo					m_stompInfo;
	std::unique_ptr<QTWebStompClient>	m_pStompClient;
	string						m_roomId;
	string						m_roomName = "videochat room";
	StreamRoomInfo				m_roomInfo;
	StreamRoomUserInfo			m_roomUserInfo;
	StreamRoomUserInfoList		m_roomUserInfoList;
	StreamRoomUpdateStatusInfo	m_roomUpdateStatusInfo;
	GuildUserInfoList			m_guildUserInfoList;
	ChattingInfo				m_chattingInfo;
	vector<GroupInfo>			m_groupInfoList;
	string						m_groupUserId;
	string						m_channelId;
	string						m_authKey; 
	eRetryConnectionState		m_retryConnectionState = eRetryConnectionState::eRCS_None;
	eStreamingState				m_streamingState = eStreamingState::eSS_None;
	string						m_limeToken = "";

	// lime login param
	string						m_gameUserID;
	string						m_token;
	string						m_deviceId;
	eServiceNetwork				m_serviceNetwork;
	string						m_appVersion;

	vector<GameUserInfo>		m_vtrCharacters;
	RetryConnectionHelper		m_retryConnectionHelper;
};