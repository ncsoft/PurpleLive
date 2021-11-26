#include "VoiceChatMediaRoom.h"

void VoiceChatMediaRoom::Init(const string& url, const string& authkey, VideoEncodingType encodeType, int framerate)
{
	m_type = MediaRoomType::MediaRoom_VoiceChat;

	__super::Init(url, authkey, encodeType, framerate);
}

void VoiceChatMediaRoom::Destroy()
{
	__super::Destroy();

}

bool VoiceChatMediaRoom::startMediaRoom()
{
	return __super::startMediaRoom();
}

void VoiceChatMediaRoom::stopMediaRoom()
{
	__super::stopMediaRoom();
}

/*
- (instancetype)initWithSignalServerURL:(NSString *)url mediaRoomAuthKey:(NSString *)token {
    
    if (self = [super initWithSignalServerURL:url mediaRoomAuthKey:token]) {
        _mediaRommType =  MediaRoom_VoiceChat;
    }
    
    return self;
}

- (void)errorOccurred:(MLiveError *)error {
    [super errorOccurred:error];
}

@end
*/