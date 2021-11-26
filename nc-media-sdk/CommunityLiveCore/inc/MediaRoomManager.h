#pragma once

#include "Singleton.h"
#include "common.h"
#include <string>

using namespace std;

class MediaRoom;
class VoiceChatMediaRoom;
class ScreenShareMediaRoom;
class WatchingMediaRoom;
class CameraCaptureMediaRoom;
class LiveStudioMediaRoom;

// singleton
class COMMUNITY_LIVE_CORE_API MediaRoomManager : public CommunityLiveCore::Singleton<MediaRoomManager>
{
public:
	MediaRoomManager();
	~MediaRoomManager();

	void					InitLog(const char* path);
	void					SetEnableLog(bool error, bool info, bool debug);
	int						GpuDetect();
	MediaRoom*				GetCurrentMediaRoom() { return m_pCurrentMediaRoom; }
	void					DestroyCurrentMediaRoom();
	// create mediaroom
	VoiceChatMediaRoom*		CreateVoiceChatRoom(const char* url, const char* token, VideoEncodingType encodeType = DEFAULT_ENCODER_TYPE, int framerate = DEFAULT_VIDEO_FRAMERATE);
	ScreenShareMediaRoom*	CreateScreenShareMediaRoom(const char* url, const char* token, VideoEncodingType encodeType = DEFAULT_ENCODER_TYPE, int framerate = DEFAULT_VIDEO_FRAMERATE);
	WatchingMediaRoom*		CreateWatchingMediaRoom(const char* url, const char* token, VideoEncodingType encodeType = DEFAULT_ENCODER_TYPE, int framerate = DEFAULT_VIDEO_FRAMERATE);
	CameraCaptureMediaRoom*	CreateCameraCaptureMediaRoom(const char* url, const char* token, VideoEncodingType encodeType = DEFAULT_ENCODER_TYPE, int framerate = DEFAULT_VIDEO_FRAMERATE);
	LiveStudioMediaRoom*	CreateLiveStudioMediaRoom(const char* url, const char* token, VideoEncodingType encodeType = DEFAULT_ENCODER_TYPE, int framerate = DEFAULT_VIDEO_FRAMERATE);

protected:
	MediaRoom*				m_pCurrentMediaRoom;
};
