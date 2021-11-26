#pragma once

#include <qstring.h>
#include <qurl.h>

#include <string>
#include <vector>

using namespace std;

const string op_domain = "https://***********";
const string sb_domain = "https://***********";
const string rc_domain = "https://***********";
const string live_domain = "https://***********";

enum eStreamingState {
	eSS_None,
	eSS_Starting,
	eSS_Streaming,
	eSS_Stopping,
	eSS_Stopped,
	eSS_Error,
};

enum eGetCharacterErrorType {
	eNoError,
	eFailFindCharacter,
	eNoStreamableCharacter,
};

enum eServiceNetwork {
	eSN_RC,
	eSN_OP,
	eSN_SB,
	eSN_LIVE,
};

enum eConnectionState {
	eCS_None,
	eCS_NeedLogIn,
	eCS_NeedConnect,
	eCS_ConnectedAll,
};

inline string get_lime_api_domain(eServiceNetwork sn)
{
	switch (sn) {
	case eSN_RC:
		return rc_domain;

	case eSN_OP:
		return op_domain;

	case eSN_SB:
		return sb_domain;

	default:
	case eSN_LIVE:
		return live_domain;
	}
}

struct MediaSignalServerInfo {
	string httpHost;
	string webSocketUrl;

	MediaSignalServerInfo()
	{
		httpHost = "";
		webSocketUrl = "";
	}
};

struct MediaRole {
	string audio;
	string video;

	MediaRole()
	{
		audio = "";
		video = "";
	}
};

struct PlayerInfo {
	string serviceType;
	string appGroupCode;
	string scope;
	string roomId;
	string playerId;
	string playerName;
	string role;
	MediaRole mediaRole;
	MediaRole mediaStatus;
	int createdTime;
	MediaSignalServerInfo mediaSignalServer;
	int keepAliveInterval;

	PlayerInfo()
	{
		serviceType = "";
		appGroupCode = "";
		scope = "";
		roomId = "";
		playerId = "";
		playerName = "";
		role = "";
		//		mediaRole;
		//		mediaStatus;
		createdTime = 0;
		//		mediaSignalServer;
		keepAliveInterval = 0;
	}
};

struct ChannelInfo {
	string groupUserId = "";
	string groupId = "";
	string channelId = "";
	string channelType = "";
	string name = "";
};

struct GroupInfo {
	string groupId = "";
	string groupType = "";
	string gameChannelType = "";
	string groupImageUrl = "";
	string name = "";
	vector<ChannelInfo> shareChannelList;
};

struct StreamRoomUserInfo {
	string roomUserId;
	string gameUserId;
	string gameCode;
	string role;
	string name;
	string intro;
	string profileImageSmall;
	string profileImageLarge;
	string userThemeColor;
	string lastMessage;
	bool banned;
	bool deported;
	bool hide;
	string serverName;
	int dateRestrictChatUntil;

	StreamRoomUserInfo()
	{
		roomUserId = "";
		gameUserId = "";
		gameCode = "";
		role = "";
		name = "";
		intro = "";
		profileImageSmall = "";
		profileImageLarge = "";
		userThemeColor = "";
		lastMessage = "";
		banned = false;
		deported = false;
		hide = false;
		serverName = "";
		dateRestrictChatUntil = 0;
	}

	bool IsUserRole() { return role=="USER"; }
	bool IsGuestRole() { return role=="GUEST"; }
};

struct StreamRoomUpdateStatusInfo {
	string streamStatus;
	int memberCount;
	int watchingCount;
	int viewCount;

	StreamRoomUpdateStatusInfo()
	{
		streamStatus = "";
		memberCount = 0;
		watchingCount = 0;
		viewCount = 0;
	}
};

struct StreamRoomUserInfoList {
	string roomId;
	int total_count;
	int loaded_count;

	vector<StreamRoomUserInfo> viewerList;

	StreamRoomUserInfoList()
	{
		roomId = "";
		total_count = 0;
		loaded_count = 0;
	}
};

struct StreamRoomInfo {
	string roomId;
	string gameCode;
	string type;
	string name;
	string description;
	int memberCount;
	int watchingCount;
	int viewCount;
	int dateCreated;
	bool closed;
	bool vodPublished;
	string topicName;
	string streamStatus;
	string thumbnailUrl;
	string inviteLinkUrl;
	int elapsedSecond;
	int closeRemainSecond;
	int dateReserved;
	int dateStreamStart;

	StreamRoomUserInfo userInfo;

	StreamRoomInfo()
	{
		roomId = "";
		gameCode = "";
		type = "";
		name = "";
		description = "";
		memberCount = 0;
		watchingCount = 0;
		viewCount = 0;
		dateCreated = 0;
		closed = false;
		vodPublished = false;
		topicName = "";
		streamStatus = "";
		thumbnailUrl = "";
		inviteLinkUrl = "";
		elapsedSecond = 0;
		closeRemainSecond = 0;
		dateReserved = 0;
		dateStreamStart = 0;
	}
};

struct GameUserInfo {
	string gameCode;
	string profileUrl;
	string gameUserId;
	string characterName;
	string serverName;
	string guildName;
	bool canStreaming;
	bool canUseGuildType;
	GameUserInfo()
	{
		gameCode = "";
		profileUrl = "";
		;
		gameUserId = "";
		characterName = "";
		;
		serverName = "";
		canStreaming = false;
		canUseGuildType = false;
		guildName = "";
	}
};

/* 추방/해제 사용자 정보 */
struct DeportUserInfo {
	string gameUserId;
	bool deport = false;
};

/* 길드 사용자 정보 */
struct GuildUserInfo {
	string groupUserId = "";
    string gameUserId = "";
    string gameCode = "";
    string characterName = "";
    string serverName = "";
    string profileImageUrl = "";
    string intro = "";
    string className = "";
    int dateLastLogout = 0;
    bool joined = false;
    string serverId = "";
    string characterId = "";
    string characterKey = "";
};
struct GuildUserInfoList {
	string guildName = "";
	string gameCode = "";
 	vector<GuildUserInfo> guildUserInfoList;
};

/* 채팅 채널 정보 */
struct ChattingGroupUserInfo {
	string groupUserId = "";
    string gameUserId = "";
    string profileImageUrl = "";
    string characterId = "";
    string characterName = "";
    string serverId = "";
    string serverName = "";
    string gameCode = "";
    string groupUserRole = "";
};
struct ChattingOwnerInfo {
	string groupUserId = "";
    string profileImageUrl = "";
    string characterName = "";
};
struct ChattingInfo{
	string groupId = "";
	ChattingGroupUserInfo groupUserInfo;
	string channelId = "";
	string groupType = "";
	string name = "";
	int memberCount = 0;
	string profileType = "";
	string gameCode = "";
	string groupImageUrl = "";
	//string lastMessageContent = "";
	//int lastMessageOccurred = 0;
	//string lastMessageChannelId = "";
	//string lastMessageGuid = "";
	//int lastMessageSeq = 0;
	//string lastMessageAttribute = "";
	//string lastMessageOptional = "";
	//string lastMessageType = "";
	//string lastMessageSubType = "";
	ChattingOwnerInfo ownerInfo;
	int maxMemberCount = 0;
	int dateCreated = 0;
	bool pushStatus = false;
	bool invitableAll = false;
};

struct StompInfo {
	string jwt;
	string topicName;
	string subscribeURL;
	string login;
	string passcode;
	string serverAppDest;
	StompInfo()
	{
		InitValue();
	}
	void InitValue()
	{
		jwt = "";
		topicName = "";
		subscribeURL = "";
		login = "";
		passcode = "";
		serverAppDest = "";
	}
};

struct ViewerInfo {
	QString name;
	QUrl profileImage;
	QString id;
	ViewerInfo()
	{
		name = "";
		profileImage = "";
		id = "";
	}
};