#include "ScreenShareMediaRoom.h"

void ScreenShareMediaRoom::Init(const string& url, const string& authkey, VideoEncodingType encodeType, int framerate)
{
	m_type = MediaRoomType::MediaRoom_ScreenShare;

	__super::Init(url, authkey, encodeType, framerate);
}

void ScreenShareMediaRoom::Destroy()
{
	__super::Destroy();

}

bool ScreenShareMediaRoom::startMediaRoom()
{
	return __super::startMediaRoom();
}

void ScreenShareMediaRoom::stopMediaRoom()
{
	__super::stopMediaRoom();
}

/*
@interface ScreenShareMediaRoom()

@property (nonatomic, strong) ScreenCaptureController *controller;
@property (nonatomic, strong) PeerConnectionClient *peerConnectionClient;
#if TARGET_IPHONE_SIMULATOR
@property (nonatomic, strong) RTCFileVideoCapturer *fileCapturer API_AVAILABLE(ios(10.0));
#endif
@end

@implementation ScreenShareMediaRoom

@synthesize mediaRommType = _mediaRommType;

- (instancetype)initWithSignalServerURL:(NSString *)url mediaRoomAuthKey:(NSString *)token {
    
    if (self = [super initWithSignalServerURL:url mediaRoomAuthKey:token]) {
        _mediaRommType =  MediaRoom_ScreenShare;
    }
    
    return self;
}
- (void)startMediaRoom {
    
    [super startMediaRoom];
    
#if TARGET_IPHONE_SIMULATOR
    if (@available(iOS 10, *)) {
        RTCFileVideoCapturer *fileCapturer = [[RTCFileVideoCapturer alloc] initWithDelegate:self.videoSource];
        
        [fileCapturer startCapturingFromFileNamed:@"demo.mp4" onError:^(NSError * _Nonnull error) {
            dispatch_async(dispatch_get_main_queue(), ^{
                if (error) {
                    
                    MLiveError *_error = [MLiveError errorWithErrorDomain:kMediaRoomErrorDomain uri:error.domain errorCode:error.code errorMessage:[NSString stringWithFormat:@"Screen recording error. \n%@", [error description]]];
                    [self errorOccurred:_error];
                }
            });
        }];
    }
#else
    ScreenCapturer *screenCapturer = [[ScreenCapturer alloc] initWithDelegate:self.videoSource];
    
    _controller = [[ScreenCaptureController alloc] initWithCapturer:screenCapturer];
    
    [_controller startCaptureWithCompletionHandler:^(NSError * _Nonnull error) {
        dispatch_async(dispatch_get_main_queue(), ^{
            if (error) {
                
                MLiveError *_error = [MLiveError errorWithErrorDomain:kMediaRoomErrorDomain uri:error.domain errorCode:error.code errorMessage:[NSString stringWithFormat:@"Screen recording error. \n%@", [error description]]];
                
                [self errorOccurred:_error];
            }
        });
    }];
#endif
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