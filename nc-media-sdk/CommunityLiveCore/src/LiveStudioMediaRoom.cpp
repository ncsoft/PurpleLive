#include "LiveStudioMediaRoom.h"

void LiveStudioMediaRoom::Init(const string& url, const string& authkey, VideoEncodingType encodeType, int framerate)
{
	m_type = MediaRoomType::MediaRoom_LiveStudio;

	__super::Init(url, authkey, encodeType, framerate);
}

void LiveStudioMediaRoom::Destroy()
{
	__super::Destroy();

}

bool LiveStudioMediaRoom::startMediaRoom()
{
	return __super::startMediaRoom();
}

void LiveStudioMediaRoom::stopMediaRoom()
{
	__super::stopMediaRoom();
}

