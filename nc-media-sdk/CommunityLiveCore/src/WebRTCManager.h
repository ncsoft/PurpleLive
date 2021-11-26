#pragma once

#include "rtc_base/win32_socket_init.h"
#include "rtc_base/win32_socket_server.h"
#include "api/peer_connection_interface.h"
#include "rtc_base/ssl_adapter.h"

#include "Singleton.h"
#include "CommunityLiveError.h"
#include "MediaRoom.h"

#include <memory>

using namespace std;
//======================================================================================
//
//	WebRTC PeerConnection 생성을 위한 초기화 작업을 담당한다.
//
//======================================================================================

#define g_WebRTC		(WebRTCManager::Instance())
#define g_WebRTC_PCF	(WebRTCManager::Instance()->GetPeerConnectionFactory())

// singleton
class WebRTCManager : public CommunityLiveCore::Singleton<WebRTCManager>
{
public:
	WebRTCManager();
	~WebRTCManager();

	webrtc::PeerConnectionFactoryInterface& GetPeerConnectionFactory();

	eErrorCode	Init(rtc::scoped_refptr<webrtc::AudioDeviceModule> adm=nullptr, VideoEncodingType encodeType=DEFAULT_ENCODER_TYPE, int framerate=DEFAULT_VIDEO_FRAMERATE);

protected:

	void	Destroy();
	bool m_bInit;

	rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> m_PeerConnFactory;

	std::unique_ptr<rtc::Thread> network_thread;
	std::unique_ptr<rtc::Thread> worker_thread;
	std::unique_ptr<rtc::Thread> signaling_thread;
};
