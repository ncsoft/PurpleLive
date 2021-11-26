#include "CameraCaptureMediaRoom.h"

void CameraCaptureMediaRoom::Init(const string& url, const string& authkey, VideoEncodingType encodeType, int framerate)
{
	m_type = MediaRoomType::MediaRoom_Camera;

	__super::Init(url, authkey, encodeType, framerate);
}

void CameraCaptureMediaRoom::Destroy()
{
	__super::Destroy();

}

bool CameraCaptureMediaRoom::startMediaRoom()
{
	return __super::startMediaRoom();
}

void CameraCaptureMediaRoom::stopMediaRoom()
{
	__super::stopMediaRoom();
}

/*
- (void)startMediaRoom {
    [super startMediaRoom];
    
    RTCCameraVideoCapturer *cameraCapturer = [[RTCCameraVideoCapturer alloc] initWithDelegate:self.videoSource];
    
    _controller = [[CameraCaptureController alloc] initWithCapturer:cameraCapturer];
    
    [_controller startCaptureWithCompletionHandler:^(NSError * error) {
        dispatch_async(dispatch_get_main_queue(), ^{
            if (error) {
                MLiveError *_error = [MLiveError errorWithErrorDomain:kMediaRoomErrorDomain uri:error.domain errorCode:error.code errorMessage:error.description];
                [self errorOccurred:_error];
            }
        });
    }];
}

- (void)stopMediaRoom {
    [super stopMediaRoom];
    
    if (_controller) {
        [_controller stopCapture];
    }
}

- (void)errorOccurred:(MLiveError *)error {
    [super errorOccurred:error];
}

@end
*/