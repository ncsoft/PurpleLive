#include "WebRTCManager.h"

#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "api/create_peerconnection_factory.h"
#include "PurpleVideoEncoderFactory.h"
#include "easylogging++.h"

#include <assert.h>

#if _MSC_VER < 1910
template <typename _BidIt>
static inline void _Reverse_tail(_BidIt _First, _BidIt _Last) throw() {
	for (; _First != _Last && _First != --_Last; ++_First) {
		const auto _Temp = *_First;
		*_First = *_Last;
		*_Last = _Temp;
	}
}

extern "C" __declspec(noalias) void __cdecl __std_reverse_trivially_swappable_8(void* _First, void* _Last) noexcept {
	_Reverse_tail(static_cast<unsigned long long *>(_First), static_cast<unsigned long long *>(_Last));
}
#endif


WebRTCManager::WebRTCManager() 
{
	m_bInit = false;
}

WebRTCManager::~WebRTCManager()
{
	Destroy();
	LOG(INFO) << "[WebRTCManager::~WebRTCManager] Singleton";
}

eErrorCode WebRTCManager::Init(rtc::scoped_refptr<webrtc::AudioDeviceModule> adm, VideoEncodingType encodeType, int framerate)
{
	if (m_bInit)
		return eErrorCode::ERROR_OK;

//	rtc::EnsureWinsockInit(); // 실제 구현 내용 없음
	rtc::WinsockInitializer winsock_init;

	network_thread = rtc::Thread::CreateWithSocketServer();
	network_thread->Start();

	worker_thread = rtc::Thread::Create();
	worker_thread->Start();

	signaling_thread = rtc::Thread::Create();
	signaling_thread->Start();

	rtc::InitializeSSL();

	if (!m_PeerConnFactory)
	{
		m_PeerConnFactory =
			webrtc::CreatePeerConnectionFactory(
				network_thread.get(),
				worker_thread.get(),
				signaling_thread.get(),
				adm, /* default_adm */
				webrtc::CreateBuiltinAudioEncoderFactory(),
				webrtc::CreateBuiltinAudioDecoderFactory(),
				webrtc::CreatePurpleVideoEncoderFactory(encodeType, framerate),
				webrtc::CreateBuiltinVideoDecoderFactory(),
				nullptr /* audio_mixer */,
				nullptr /* audio_processing */
			);

		if (!m_PeerConnFactory) 
		{
			return eErrorCode::WEBRTC_PeerConnectionFactoryCreateFailed;
		}
	}

	m_bInit = true;
	return eErrorCode::ERROR_OK;
}

void WebRTCManager::Destroy()
{
	if (!m_bInit)
		return;

	if (m_PeerConnFactory) {
		m_PeerConnFactory = nullptr;
	}

	if (!network_thread->IsCurrent())
		network_thread->Stop();
	if (!worker_thread->IsCurrent())
		worker_thread->Stop();
	if (!signaling_thread->IsCurrent())
		signaling_thread->Stop();

	network_thread.release();
	worker_thread.release();
	signaling_thread.release();

	rtc::CleanupSSL();

	return;
}

webrtc::PeerConnectionFactoryInterface& WebRTCManager::GetPeerConnectionFactory()
{
	if (!m_bInit) 
	{
		ELOGE << "PeerConnectionFactory wrong usage";
		assert(m_bInit);
	}

	return *m_PeerConnFactory.get();
}
