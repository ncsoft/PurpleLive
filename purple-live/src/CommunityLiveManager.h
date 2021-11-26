#pragma once

//#include "Singleton.h"
//#include "jsoncpp/json/json.h"
#include "MediaRoom.h"
#include "CommunityLiveCore.h"
#include "CommunityLiveError.h"
#include <windows.h>
#include <thread>
#include <mutex>
#include <string>
#include "RetryConnectionHelper.h"

#define MAX_AV_PLANES 8
struct video_data {
	uint8_t           *data[MAX_AV_PLANES];
	uint32_t          linesize[MAX_AV_PLANES];
	uint64_t          timestamp;
};

struct audio_data {
	uint8_t             *data[MAX_AV_PLANES];
	uint32_t            frames;
	uint64_t            timestamp;
};

struct live_config {
	std::string signal_url;
	std::string auth_key;
	std::string scope;
	std::string room_name;
	std::string player_name;
};

struct media_config {
	int framerate = 30;
	int bitrate = 10000000;
	VideoEncodingType encoding_type = VET_SOFTWARE;
};

class CommunityLiveManager : public CommunityLiveCore::Singleton<CommunityLiveManager>
{
public:
	CommunityLiveManager();
	virtual ~CommunityLiveManager();

	static int GpuDetect();

	void SetLiveConfig(live_config& config);
	void SetMediaConfig(media_config& config);
	void SetEnableStatisticsLog(bool enable);

	bool MediaRoomRegister();
	void MediaRoomJoin(const char* webSocketURL = nullptr, const char* authKey = nullptr);
	void MediaRoomStop();

	void OnVideoFrame(void* frame, uint32_t width, uint32_t height, bool rgb=false);
	void OnAudioFrame(void* frame);

	video_data* GetVideoData();
	audio_data* GetAudioData();
	
	eRetryConnectionState GetRetryConnectionState();
	void SetRetryConnectionState(eRetryConnectionState state);

	void CheckRetryConnection();
	void StopRetryConnection();
	void ReconnectSignalServer();

private:
	MediaRoom* m_pMediaRoom = nullptr;

	std::string m_strURL;
	std::string m_strAuthKey;

	live_config m_live_config;
	media_config m_media_config;
	
	video_data m_vdata;
	audio_data m_adata;

	RetryConnectionHelper m_retryConnectionHelper;
	eRetryConnectionState m_retryConnectionState = eRetryConnectionState::eRCS_None;
};