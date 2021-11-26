#include "MediaRoomManager.h"
#include "MediaRoom.h"
#include "VoiceChatMediaRoom.h"
#include "ScreenShareMediaRoom.h"
#include "WatchingMediaRoom.h"
#include "CameraCaptureMediaRoom.h"
#include "LiveStudioMediaRoom.h"
#include "HardwareEncoderDetector.h"
#include "easylogging++.h"

MediaRoomManager::MediaRoomManager()
{
	m_pCurrentMediaRoom = nullptr;
}

MediaRoomManager::~MediaRoomManager()
{
	DestroyCurrentMediaRoom();
	LOG(INFO) << "[MediaRoomManager::~MediaRoomManager] Singleton";
}

void MediaRoomManager::InitLog(const char* path)
{
	elpp_set_path(path);
}

void MediaRoomManager::SetEnableLog(bool error, bool info, bool debug)
{
	elpp_set_enable_log(error, info, debug);
}

int MediaRoomManager::GpuDetect()
{
	return HardwareEncoderDetector::detect();
}

VoiceChatMediaRoom* MediaRoomManager::CreateVoiceChatRoom(const char* url, const char* token, VideoEncodingType encodeType, int framerate)
{
	DestroyCurrentMediaRoom();

	m_pCurrentMediaRoom = new VoiceChatMediaRoom();
	m_pCurrentMediaRoom->Init(url, token, encodeType, framerate);

	return static_cast<VoiceChatMediaRoom*>(m_pCurrentMediaRoom);
}

ScreenShareMediaRoom* MediaRoomManager::CreateScreenShareMediaRoom(const char* url, const char* token, VideoEncodingType encodeType, int framerate)
{
	DestroyCurrentMediaRoom();

	m_pCurrentMediaRoom = new ScreenShareMediaRoom();
	m_pCurrentMediaRoom->Init(url, token, encodeType, framerate);

	return static_cast<ScreenShareMediaRoom*>(m_pCurrentMediaRoom);
}

WatchingMediaRoom* MediaRoomManager::CreateWatchingMediaRoom(const char* url, const char* token, VideoEncodingType encodeType, int framerate)
{
	DestroyCurrentMediaRoom();

	m_pCurrentMediaRoom = new WatchingMediaRoom();
	m_pCurrentMediaRoom->Init(url, token, encodeType, framerate);

	return static_cast<WatchingMediaRoom*>(m_pCurrentMediaRoom);
}

CameraCaptureMediaRoom* MediaRoomManager::CreateCameraCaptureMediaRoom(const char* url, const char* token, VideoEncodingType encodeType, int framerate)
{
	DestroyCurrentMediaRoom();

	m_pCurrentMediaRoom = new CameraCaptureMediaRoom();
	m_pCurrentMediaRoom->Init(url, token, encodeType, framerate);

	return static_cast<CameraCaptureMediaRoom*>(m_pCurrentMediaRoom);
}
LiveStudioMediaRoom* MediaRoomManager::CreateLiveStudioMediaRoom(const char* url, const char* token, VideoEncodingType encodeType, int framerate)
{
	DestroyCurrentMediaRoom();

	m_pCurrentMediaRoom = new LiveStudioMediaRoom();
	m_pCurrentMediaRoom->Init(url, token, encodeType, framerate);

	return static_cast<LiveStudioMediaRoom*>(m_pCurrentMediaRoom);
}

void MediaRoomManager::DestroyCurrentMediaRoom()
{
	if (m_pCurrentMediaRoom != nullptr)
	{
		delete m_pCurrentMediaRoom;
		m_pCurrentMediaRoom = nullptr;
	}
}

