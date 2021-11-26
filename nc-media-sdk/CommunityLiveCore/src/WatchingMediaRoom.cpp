#include "WatchingMediaRoom.h"

void WatchingMediaRoom::Init(const string& url, const string& authkey, VideoEncodingType encodeType, int framerate)
{
	m_type = MediaRoomType::MediaRoom_Watching;

	__super::Init(url, authkey, encodeType, framerate);
}

void WatchingMediaRoom::Destroy()
{
	__super::Destroy();

}

bool WatchingMediaRoom::startMediaRoom()
{
	return __super::startMediaRoom();
}

void WatchingMediaRoom::stopMediaRoom()
{
	__super::stopMediaRoom();
}
/*
- (instancetype)initWithSignalServerURL:(NSString *)url mediaRoomAuthKey:(NSString *)token {
    
    if (self = [super initWithSignalServerURL:url mediaRoomAuthKey:token]) {
        _mediaRommType =  MediaRoom_Watching;
        
        self.remoteView = [[BroadcastRemoteVideoView alloc] initWithFrame:CGRectZero];
    }
    
    return self;
}

- (void)errorOccurred:(MLiveError *)error {
    [super errorOccurred:error];
}

- (PeerConnectionClient *)registerPeerConnectionClientWithPeerId:(NSString *)peerId {
    
    PeerConnectionClient *peerConnectionClient = [super registerPeerConnectionClientWithPeerId:peerId];
    peerConnectionClient.remoteVideoView = self.remoteView;
    
    return peerConnectionClient;
}
@end
*/