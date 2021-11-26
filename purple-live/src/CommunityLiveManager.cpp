#include "CommunityLiveManager.h"
#include "easylogging++.h"
#include "json.h"
#include "HttpUtil.h"
#include <mutex>
#include <sstream>
#include <iostream>
#include "AppConfig.h"
#include "AppString.h"
#include "AppUtils.h"
#include "MainWindow.h"

#ifdef _DEBUG
	#pragma comment(lib, "jsoncpp-vc140d.lib")
	#pragma comment(lib, "CommunityLiveCore.lib")
#else
	#pragma comment(lib, "jsoncpp-vc140.lib")
	#pragma comment(lib, "CommunityLiveCore.lib")
#endif


CommunityLiveManager::CommunityLiveManager()
{
	MediaRoomManager::Instance()->InitLog(AppUtils::GetLogPath().c_str());
	MediaRoomManager::Instance()->SetEnableLog(AppConfig::enable_log_error, AppConfig::enable_log_info, AppConfig::enable_log_debug);
}

CommunityLiveManager::~CommunityLiveManager()
{
	elogd("[CommunityLiveManager::~CommunityLiveManager] Singleton");
}

int CommunityLiveManager::GpuDetect()
{
	return MediaRoomManager::Instance()->GpuDetect();
}

void CommunityLiveManager::SetLiveConfig(live_config& lconfig)
{
	LOG(DEBUG) << format_string("[CommunityLiveManager::SetLiveConfig]");
	m_live_config = lconfig;
}

void CommunityLiveManager::SetMediaConfig(media_config& mconfig)
{
	LOG(DEBUG) << format_string("[CommunityLiveManager::SetMediaConfig]");
	m_media_config = mconfig;
}

void CommunityLiveManager::SetEnableStatisticsLog(bool enable)
{
	LOG(DEBUG) << format_string("[CommunityLiveManager::SetEnableStatisticsLog]");
	if (m_pMediaRoom)
	{	
		int64_t result;
		m_pMediaRoom->ProcessCommand(MediaRoomCommand::SetEnableStatObserver, enable, result);
		m_pMediaRoom->ProcessCommand(MediaRoomCommand::SetEnableStatLog, enable, result);
	}
}

static void OnMediaRoomDelegate(void* owner, int32_t msgType, const char *domain, const char *uri, int32_t error, const char *msg)
{
	switch (msgType) {
	case 0: //MT_Unknown
		LOG(INFO) << format_string("[CommunityLiveManager::OnMediaRoomDelegate] msgType(%d) domain(%s), uri(%s) error(%d) msg(%s)", msgType, domain, uri, error, msg);
		break;
	case 1: //MT_Notify
		LOG(INFO) << format_string("[CommunityLiveManager::OnMediaRoomDelegate] msgType(%d) domain(%s), uri(%s) error(%d) msg(%s)", msgType, domain, uri, error, msg);
		break;
	case 2: //MT_Error
		{
			LOG(ERROR) << format_string("[CommunityLiveManager::OnMediaRoomDelegate] msgType(%d) domain(%s), uri(%s) error(%d) msg(%s)", msgType, domain, uri, error, msg);
			if (string(domain) == DOMAIN_MEDIAROOM)
			{
				MainWindow::Get()->OnMediaSeverError(error, msg);
			}
			else if (string(domain) == DOMAIN_SIGNAL_CLIENT)
			{
				bool disconnect = (string(uri)==URI_SIGNAL_CLIENT_CONNECTION) && (error != 0);
				if (disconnect)
				{
					AppUtils::DispatchToMainThread([] {
						CommunityLiveManager::Instance()->CheckRetryConnection();
					});
				}
				else
				{
					MainWindow::Get()->OnSignalServerError(error, msg);
				}
			}
		}
		break;
	default:
		LOG(INFO) << format_string("[CommunityLiveManager::OnMediaRoomDelegate] msgType(%d) domain(%s), uri(%s) error(%d) msg(%s)", msgType, domain, uri, error, msg);
	}
}

void MakeJsonMsg(Json::Value& jMsg, live_config lconfig)
{
	std::string auth_key = lconfig.auth_key;
	std::string service_type = "video_broadcast";
	std::string room_id = "video_broadcast";
	
	std::string scope = lconfig.scope;
	
	std::string role = "host";
	std::string audio = "sendrecv";
	std::string video = "sendrecv";
	
	std::string room_name = lconfig.room_name;
	std::string player_name = lconfig.player_name;
	
	jMsg["auth_key"]			= auth_key;
	jMsg["service_type"]		= service_type;
	jMsg["room_id"]				= room_id;
	jMsg["scope"]				= scope;
	jMsg["player_name"]			= player_name;
	jMsg["role"]				= role;
	jMsg["media_role"]["audio"] = audio;
	jMsg["media_role"]["video"] = video;
	jMsg["auto_create_room"]["room_name"] = room_name;
	jMsg["auto_create_room"]["destroy_when_exists"] = TRUE;

	Json::Value & jevts = jMsg["auto_create_room"]["allowed_events"] = Json::arrayValue;
	jevts[0] = "join_stream";
	jevts[1] = "leave_room";
}

bool CommunityLiveManager::MediaRoomRegister()
{
	LOG(DEBUG) << format_string("[CommunityLiveManager::MediaRoomRegister]");

	string url = m_live_config.signal_url;
	wstring uri = L"/media_signal/register_player";

	Json::Value jMsg;
	MakeJsonMsg(jMsg, m_live_config);

	Json::StreamWriterBuilder writer;
	string msg = Json::writeString(writer, jMsg);
	OutputDebugStringA(msg.c_str());

	bool result = HttpPost(url, uri, msg, [=](const string& _msg) {
		stringstream strmbody(_msg);
		Json::CharReaderBuilder rbuilder;
		Json::Value jrsp;
		string errs;
		if (!Json::parseFromStream(rbuilder, strmbody, &jrsp, &errs))
		{
			return false;
		}

		if (jrsp["auth_key"].empty())
		{
			return false;
		}

		if (jrsp["media_signal_server"]["web_socket_url"].empty())
		{
			return false;
		}

		m_strAuthKey = jrsp["auth_key"].asCString();
		m_strURL = jrsp["media_signal_server"]["web_socket_url"].asCString();
		return true;
	});

	return result;
}

void CommunityLiveManager::MediaRoomJoin(const char* webSocketURL, const char* authKey)
{
	LOG(DEBUG) << format_string("[CommunityLiveManager::MediaRoomJoin]");

	if (m_pMediaRoom)
		return;

	std::string type = "video_broadcast";
	
	if (type == "audio_broadcast" || type == "p2p")
	{
		m_pMediaRoom = (MediaRoom*)MediaRoomManager::Instance()->CreateVoiceChatRoom(m_strURL.c_str(), m_strAuthKey.c_str());
	}
	else if (type == "video_broadcast" || type == "sfu")
	{
		m_pMediaRoom = (MediaRoom*)MediaRoomManager::Instance()->CreateLiveStudioMediaRoom(m_strURL.c_str(), m_strAuthKey.c_str(), m_media_config.encoding_type, m_media_config.framerate);
		m_pMediaRoom->ProcessCommand(MediaRoomCommand::SetMaxVideoBitrates, m_media_config.bitrate);
#if USE_STREAMING_STATS_INFO_LOG
		m_pMediaRoom->ProcessCommand(MediaRoomCommand::SetEnableStatObserver, true);
		m_pMediaRoom->ProcessCommand(MediaRoomCommand::SetEnableStatLog, true);
#endif
	}
	
	if (m_pMediaRoom)
	{
		m_pMediaRoom->SetDelegate(OnMediaRoomDelegate, (void *)this);

		if (webSocketURL && authKey)
		{
			LOG(DEBUG) << format_string("[CommunityLiveManager::MediaRoomJoin] SetConnectionInfo(%s, %s)", webSocketURL, authKey);
			m_pMediaRoom->SetConnectionInfo(webSocketURL, authKey);
		}

		LOG(INFO) << format_string("[CommunityLiveManager::MediaRoomJoin] startMediaRoom()");
		if (m_pMediaRoom->startMediaRoom() == false)
		{
			LOG(DEBUG) << format_string("[CommunityLiveManager::MediaRoomJoin] startMediaRoom() failed");
			m_pMediaRoom->stopMediaRoom();
			MediaRoomManager::Instance()->DestroyCurrentMediaRoom();
			m_pMediaRoom = NULL;
		}
	}
}

void CommunityLiveManager::MediaRoomStop()
{
	LOG(DEBUG) << format_string("[CommunityLiveManager::MediaRoomStop]");

	if (m_pMediaRoom)
	{
		LOG(INFO) << format_string("[CommunityLiveManager::MediaRoomStop] stopMediaRoom");
		m_pMediaRoom->stopMediaRoom();
		MediaRoomManager::Instance()->DestroyCurrentMediaRoom();
		m_pMediaRoom = NULL;
	}
}

void CommunityLiveManager::OnVideoFrame(void* frame, uint32_t width, uint32_t height, bool rgb)
{
	if(m_pMediaRoom)
		m_pMediaRoom->OnVideoFrame(frame, width, height, rgb);
}
	
void CommunityLiveManager::OnAudioFrame(void* frame)
{
	if(m_pMediaRoom)
		m_pMediaRoom->OnAudioFrame(frame);
}

video_data* CommunityLiveManager::GetVideoData()
{
	return &m_vdata;
}

audio_data* CommunityLiveManager::GetAudioData()
{
	return &m_adata;
}

eRetryConnectionState CommunityLiveManager::GetRetryConnectionState()
{
	return m_retryConnectionState;
}

void CommunityLiveManager::SetRetryConnectionState(eRetryConnectionState state)
{
	LOG(INFO) << format_string("[CommunityLiveManager::SetRetryConnectState] eRetryConnectState(%d)", state);
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
			MainWindow::Get()->AppClose(AppString::GetString("error_message/error_signal_server_reconnect_failed").c_str());
		});
		break;
	default:
		break;
	}

	m_retryConnectionState = state;
}

void CommunityLiveManager::CheckRetryConnection()
{
	elogd("[CommunityLiveManager::CheckRetryConnect]");
	if (!m_retryConnectionHelper.IsRetrying())
	{
		// Signal 서버 재연결 시도 정책 : 2초 간격으로 3번 시도
		m_retryConnectionHelper.SetRetryTimes({ 2, 2, 2 });
		m_retryConnectionHelper.Init();
		SetRetryConnectionState(eRetryConnectionState::eRCS_Retrying);
	}

	m_retryConnectionHelper.Retry([this](bool timeout) {
		if (timeout)
			SetRetryConnectionState(eRetryConnectionState::eRCS_Failed);
		else
			ReconnectSignalServer();
	});
}

void CommunityLiveManager::StopRetryConnection()
{
	elogd("[CommunityLiveManager::StopRetryConnect]");
	if (m_retryConnectionHelper.IsRetrying())
		m_retryConnectionHelper.Stop();
}

void CommunityLiveManager::ReconnectSignalServer()
{
	eloge("[CommunityLiveManager::ReconnectLiveWebServer]");
	if (MediaRoomManager::Instance()->GetCurrentMediaRoom()->ReconnectSignal())
	{
		SetRetryConnectionState(eRetryConnectionState::eRCS_Successed);
	}	
	else
	{
		CheckRetryConnection();
	}
}