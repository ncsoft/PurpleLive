#include "MediaRoomBase.h"
#include "MediaRoomManager.h"
#include <vector>
#include "SignalClient.h"
#include "PeerConnection.h"
#include "WebRTCManager.h"
#include "WebRTCHelper.h"
#include "CommunityLiveError.h"
#include "VideoCapturer.h"
#include <third_party/libyuv/include/libyuv.h>
#include "easylogging++.h"
#include "util.h"
#include "EncoderStatistics.h"
#include "avc_util.hpp"

MediaRoomBase::MediaRoomBase()
{
	m_pSignalClient = nullptr;
	m_pMediaRoomDelegate = nullptr;
	m_pMediaSource = new WebRTCMediaSource();
	m_nFrameRate = DEFAULT_VIDEO_FRAMERATE;
	m_nMaxVideoBitrates = DEFAULT_MAX_VIDEO_BITRATE;

	if(CommunityLiveCoreConfig::USE_STAT_INFO) {
		m_mediaStatInfo.setEnableObserver(false);
		m_bStatThreadStarted = false;
		m_bStatThreadStop = false;
	}

#if USE_ENCODER_PERFORMANCE_TEST
	m_mediaStatInfo.setEnableObserver(true);
#endif
}

MediaRoomBase::~MediaRoomBase()
{
	m_pMediaRoomDelegate = nullptr;
	delete m_pMediaSource;
	Destroy();
}

void MediaRoomBase::Init(const string& url, const string& authkey, VideoEncodingType encodeType, int framerate)
{
	elogi("[MediaRoomBase::Init] type(%d) encodeType(%d) framerate(%d)", m_type, encodeType, framerate);

	// WebRTC 초기화
	eErrorCode errorCode;

	if (m_type == MediaRoom_LiveStudio)
	{
		m_AudioDeviceModule.AddRef();
		errorCode = g_WebRTC->Init(&m_AudioDeviceModule, encodeType, framerate);
	}
	else
		errorCode = g_WebRTC->Init();

	if (errorCode != eErrorCode::ERROR_OK)
	{
		CallOnError(DOMAIN_MEDIAROOM, URI_PEER_CONNECTION, errorCode, "WebRTCManager::Init()");
		return;
	}

	m_signalServerUrl = url;
	m_authKey = authkey;

	m_bUseMic		= true;
	m_bUseSpeaker	= true;
	m_bStarted		= false;
	m_bReconnection	= false;
	m_nKeepAliveInterval = DEFAULT_KEEP_ALIVE_INTERVAL;

	if (m_type != MediaRoom_Watching)
	{
		rtc::scoped_refptr<webrtc::MediaStreamInterface> stream = g_WebRTC_PCF.CreateLocalMediaStream("community_live");

		cricket::AudioOptions options;
		options.echo_cancellation.emplace(false);	// default : true
		options.auto_gain_control.emplace(false);	// default : true
		options.noise_suppression.emplace(false);	// default : true
		options.highpass_filter.emplace(false);		// default : true
		options.stereo_swapping.emplace(false);
		options.typing_detection.emplace(false);	// default : true
		options.experimental_ns.emplace(false);		// default : true
		options.experimental_ns.emplace(false);
		options.residual_echo_detector.emplace(false);	// default : true

		options.audio_jitter_buffer_max_packets.emplace(false);

//		options.aecm_generate_comfort_noise.emplace(false);
//		options.delay_agnostic_aec.emplace(false);

		m_pMediaSource->m_audioSource = g_WebRTC_PCF.CreateAudioSource(options);
	}

	if (m_type == MediaRoom_LiveStudio)
	{
		m_pMediaSource->m_videoSource = new rtc::RefCountedObject<VideoCapturer>();
	}
}

void MediaRoomBase::Destroy()
{
	ELOGD << format_string("[MediaRoomBase::Destroy]");

	if(CommunityLiveCoreConfig::USE_STAT_INFO) {
		if(CommunityLiveCoreConfig::USE_STAT_THREAD) {
			StopStatThread();
		}
	}

	RemoveAllPeerConnection();
	if (m_pSignalClient)
	{
		delete m_pSignalClient;
		m_pSignalClient = nullptr;
	}

	g_WebRTC->DestroyInstance();

	ELOGI << format_string("[MediaRoomBase::Destroy] end");
}

bool MediaRoomBase::startMediaRoom()
{
	ELOGI << format_string("[MediaRoomBase::startMediaRoom]");

	if (m_bStarted)
	{
		return true;
	}

	m_bStarted = true;

	// create websocket
	if (m_pSignalClient == nullptr)
	{
		m_pSignalClient = new SignalClient(this);
	}

	//ACTION : JOIN
	if (m_pSignalClient->JoinRoom(m_signalServerUrl, m_authKey) == false)
	{
		CallOnError(DOMAIN_MEDIAROOM, URI_START_MEDIA_ROOM, eErrorCode::ConnectionFailed, "Connection Failed to Signal Server");
		return false;
	}

	if(CommunityLiveCoreConfig::USE_STAT_INFO) {
		m_mediaStatInfo.initialize();

		if(CommunityLiveCoreConfig::USE_STAT_THREAD) {
			StartStatThread();
		}
	}

	ELOGD << format_string("[MediaRoomBase::startMediaRoom] end");

	return true;
}

void MediaRoomBase::stopMediaRoom()
{
	ELOGD << format_string("[MediaRoomBase::stopMediaRoom]");
	
	if(m_bStatBlocking) {
		ELOGD << format_string("\n\n\n\n\n[MediaRoomBase::stopMediaRoom] GetStat Blocking!!!!\n\n\n\n\n");
	}

	m_bStarted = false;
	if(CommunityLiveCoreConfig::USE_STAT_THREAD) {
		StopStatThread();
	}
	CallOnStopped();
//	Destroy();

	ELOGI << format_string("[MediaRoomBase::stopMediaRoom] end");
}

void MediaRoomBase::StartStatThread()
{
	if(m_bStatThreadStarted)
		return;

	m_statThread = std::thread([](MediaRoomBase* pMediaRoom){
		while(true) {
			if(pMediaRoom->m_bStatThreadStop)
				break;
			pMediaRoom->UpdateStatInfo();
			std::this_thread::sleep_for(std::chrono::milliseconds(CommunityLiveCoreConfig::TIMER_GET_STAT_INTERVAL));
		}
		pMediaRoom->m_bStatThreadStop = false;
		pMediaRoom->m_bStatThreadStarted = false;
	}, this);
}

void MediaRoomBase::StopStatThread()
{
	m_bStatThreadStop = true;
	if (m_statThread.joinable())
		m_statThread.join();
}

bool MediaRoomBase::GetEnableStatObserver()
{
	return m_mediaStatInfo.getEnableObserver();
}

void MediaRoomBase::SetEnableStatObserver(bool enable)
{
	m_mediaStatInfo.setEnableObserver(enable);
	
	if(CommunityLiveCoreConfig::USE_STAT_INFO)
	{
		for (auto itr = m_mapPeerConnection.begin(); itr != m_mapPeerConnection.end(); itr++)
		{
			PeerConnection* pPeerConnection = (*itr).second;
			if (pPeerConnection)
			{
				pPeerConnection->SetEnableStatObserver(enable);
			}
		}
	}
}

void MediaRoomBase::SetEnableStatLog(bool enable)
{
	elogi("[MediaRoomBase::SetEnableStatLog] enable(%d)", enable);
	m_mediaStatInfo.setEnableLog(enable);
}

void MediaRoomBase::UpdateStatInfo()
{
	if(!m_bStarted)
		return;

	if(!m_mediaStatInfo.getEnableObserver())
		return;

	if(CommunityLiveCoreConfig::USE_STAT_INFO)
	{
		for (auto itr = m_mapPeerConnection.begin(); itr != m_mapPeerConnection.end(); itr++)
		{
			PeerConnection* pPeerConnection = (*itr).second;
			if (pPeerConnection && pPeerConnection->GetEnableStatObserver())
			{
				pPeerConnection->GetStatObserver()->SetEnableLog(m_mediaStatInfo.getEnableLog());

				//static int __getStatCount = 0;
				//ELOGD << format_string("%%%%%%%%%%%%%%% GetPeerConnection()->GetStats start  (%d)", __getStatCount);
				m_bStatBlocking = true;
				pPeerConnection->GetPeerConnection()->GetStats(pPeerConnection->GetStatObserver());
				m_bStatBlocking = false;
				////ELOGD << format_string("%%%%%%%%%%%%%%% GetPeerConnection()->GetStats finish (%d)", __getStatCount++);
			}
		}
	}
}

int64_t MediaRoomBase::ProcessCommand(int32_t command, int32_t type)
{
	int64_t result = 0;
	ProcessCommand(command, type, result);
	return result;
}

bool MediaRoomBase::ProcessCommand(int32_t command, int32_t type, int64_t& result)
{
	elogd("[MediaRoomBase::ProcessCommand] command(%d) type(%d)", command, type);

	result = 0;
	MediaRoomCommand commandType = static_cast<MediaRoomCommand>(command);
	switch(commandType) 
	{
	case MediaRoomCommand::GetEnableStatObserver:
		result = GetEnableStatObserver();
		break;
	case MediaRoomCommand::SetEnableStatObserver:
		SetEnableStatObserver(static_cast<bool>(type));
		break;
	case MediaRoomCommand::SetEnableStatLog:
		SetEnableStatLog(static_cast<bool>(type));
		break;
	case MediaRoomCommand::GetSentBytes:
		if(m_mediaStatDetailInfo.bytes_sent > 0) {
			result = m_mediaStatDetailInfo.bytes_sent;
		}
		else {
			result = m_mediaStatInfo.audio_bytes_sent + m_mediaStatInfo.video_bytes_sent;
		}
		break;
	case MediaRoomCommand::GetFrameSent:
		result = m_mediaStatInfo.frames_sent;
		break;
	case MediaRoomCommand::GetFrameDropped:
		result = m_mediaStatInfo.frames_dropped;
		break;
	case MediaRoomCommand::GetMediaStatDetail:
		result = reinterpret_cast<int64_t>(&m_mediaStatDetailInfo);
		break;
	case MediaRoomCommand::GetFrameRate:
		result = m_nFrameRate;
		break;
	case MediaRoomCommand::SetFrameRate:
		m_nFrameRate = type;
		break;
	case MediaRoomCommand::GetMaxVideoBitrates:
		result = m_nMaxVideoBitrates;
		break;
	case MediaRoomCommand::SetMaxVideoBitrates:
		m_nMaxVideoBitrates = type;
		break;
	case MediaRoomCommand::SetEncoderQuilityPreset:
		g_cec->quality_preset = static_cast<EncoderQualityPreset>(type);
		break;
	case MediaRoomCommand::SetEncoderRateControl:
		g_cec->rate_control = static_cast<EncoderRateControl>(type);
		break;
	case MediaRoomCommand::SetEnableEncoderLogFile:
		g_es->SetEnableEncoderLogFile(static_cast<bool>(type));
		break;
	case MediaRoomCommand::SetIntervalEncoderLogFile:
		g_es->SetIntervalEncoderLogFile(static_cast<int>(type));
		break;
	default:
		return false;
	}
	return true;
}

void MediaRoomBase::SetMicEnable(bool enable)
{
	elogi("[MediaRoomBase::SetMicEnable] enable(%d)", enable);

	MediaRoom::SetMicEnable(enable);

	for (auto itr = m_mapPeerConnection.begin(); itr != m_mapPeerConnection.end(); itr++)
	{
		PeerConnection* pPeerConnection = (*itr).second;
		if (pPeerConnection)
		{
			pPeerConnection->SetMicEnable(m_bUseMic);
		}
	}
}

void MediaRoomBase::SetSpeakerEnable(bool enable)
{
	elogi("[MediaRoomBase::SetSpeakerEnable] enable(%d)", enable);

	MediaRoom::SetSpeakerEnable(enable);

	for (auto itr = m_mapPeerConnection.begin(); itr != m_mapPeerConnection.end(); itr++)
	{
		PeerConnection* pPeerConnection = (*itr).second;
		if (pPeerConnection)
		{
			pPeerConnection->SetMicEnable(m_bUseSpeaker);
		}
	}
}

bool MediaRoomBase::ReconnectSignal()
{
	elogi("[MediaRoomBase::ReconnectSignal]");

	delete m_pSignalClient;
	m_pSignalClient = nullptr;
	if (m_pSignalClient == nullptr)
	{
		m_pSignalClient = new SignalClient(this);
	}

	return m_pSignalClient->_Connect(m_signalServerUrl, m_authKey);
}

// PeerConnection 조회(없으면 생성)
PeerConnection*	MediaRoomBase::FindPeerConnection(const string& peerID, bool bCreateIfNotExist, const vector<IceServerInfo>* pIceServerList/*=nullptr*/)
{
	MAP_PEERCONNECTION_ITR itr = m_mapPeerConnection.find(peerID);

	if (itr == m_mapPeerConnection.end())
	{
		if (bCreateIfNotExist)
		{
			PeerConnectionMediaType mediaType = m_type == MediaRoom_VoiceChat ? PeerConnection_Voip : PeerConnection_Video;
			PeerConnection* pNewPeerConnection = new PeerConnection(peerID, mediaType, 
				*m_pMediaSource->m_audioSource.get(), *m_pMediaSource->m_videoSource.get(), pIceServerList);
			if (pNewPeerConnection == nullptr)
			{
				// ERROR LOG
				return nullptr;
			}
			else
			{
				m_mapPeerConnection.insert(pair<string, PeerConnection*>(peerID, pNewPeerConnection));
				return pNewPeerConnection;
			}
		}
		else
		{
			// TODO : "cannot found peerConnection" LOG
			return nullptr;
		}
	}
	else
		return (*itr).second;
}

void MediaRoomBase::RemovePeerConnection(const string& peerID)
{
	MAP_PEERCONNECTION_ITR itr = m_mapPeerConnection.find(peerID);
	if (itr == m_mapPeerConnection.end())
	{
		// ERROR LOG
		// can not find peerconnection
	}
	else
	{
		PeerConnection* pPeerConnection = (*itr).second;
		m_mapPeerConnection.erase(itr);

		delete pPeerConnection;
	}
}

void MediaRoomBase::RemoveAllPeerConnection()
{
	for (auto itr = m_mapPeerConnection.begin(); itr != m_mapPeerConnection.end(); itr++)
	{
		PeerConnection* pPeerConnection = (*itr).second;
		delete pPeerConnection;
	}

	m_mapPeerConnection.clear();
}

void MediaRoomBase::OnVideoFrame(void* frame, uint32_t width, uint32_t height, bool rgb)
{
	video_data* pFrame = (video_data*)frame;
	if (!frame)
		return;

	if (!m_pMediaSource->m_videoSource)
		return;

	//Calculate size
	m_VideoCaptureCapability.width = width;
	m_VideoCaptureCapability.height = height;

	if (rgb == true)
		m_VideoCaptureCapability.videoType = webrtc::VideoType::kARGB;
	else
		m_VideoCaptureCapability.videoType = webrtc::VideoType::kNV12;

	// just for count capture
	g_es->CaptureFrameStart();		
	g_es->CaptureFrameEnd();

	//Pass it
	((VideoCapturer*)(m_pMediaSource->m_videoSource.get()))->OnFrameCaptured(pFrame->data[0], m_VideoCaptureCapability);

	if(CommunityLiveCoreConfig::USE_STAT_INFO) {
		if(!CommunityLiveCoreConfig::USE_STAT_THREAD) {
			UpdateStatInfo();
		}
	}

/*
	//If we are doing thumbnails and we are not skiping this frame
	if (thumbnail && thumbnailCapture && ((picId % thumbnailDownrate) == 0))
	{

		//Calculate size
		thumbnailCaptureCapability.width = obs_output_get_width(output) / thumbnailDownscale;
		thumbnailCaptureCapability.height = obs_output_get_height(output) / thumbnailDownscale;
		thumbnailCaptureCapability.videoType = webrtc::VideoType::kI420;
		//Calc size of the downscaled version
		uint32_t donwscaledSize = thumbnailCaptureCapability.width*thumbnailCaptureCapability.height * 3 / 2;
		//Create new image for the I420 conversion and the downscaling
		uint8_t* downscaled = (uint8_t*)malloc(size + donwscaledSize);

		//Get planar pointers
		uint8_t *dest_y = (uint8_t *)downscaled;
		uint8_t *dest_u = (uint8_t *)dest_y + m_VideoCaptureCapability.width*m_VideoCaptureCapability.height;
		uint8_t *dest_v = (uint8_t *)dest_y + m_VideoCaptureCapability.width*m_VideoCaptureCapability.height * 5 / 4;
		uint8_t *resc_y = (uint8_t *)downscaled + size;
		uint8_t *resc_u = (uint8_t *)resc_y + thumbnailCaptureCapability.width*thumbnailCaptureCapability.height;
		uint8_t *resc_v = (uint8_t *)resc_y + thumbnailCaptureCapability.width*thumbnailCaptureCapability.height * 5 / 4;

		//Convert first to I420
		libyuv::NV12ToI420(
			(uint8_t *)frame->data[0], frame->linesize[0],
			(uint8_t *)frame->data[1], frame->linesize[1],
			dest_y, m_VideoCaptureCapability.width,
			dest_u, m_VideoCaptureCapability.width / 2,
			dest_v, m_VideoCaptureCapability.width / 2,
			m_VideoCaptureCapability.width, m_VideoCaptureCapability.height);

		//Rescale
		libyuv::I420Scale(
			dest_y, m_VideoCaptureCapability.width,
			dest_u, m_VideoCaptureCapability.width / 2,
			dest_v, m_VideoCaptureCapability.width / 2,
			m_VideoCaptureCapability.width, m_VideoCaptureCapability.height,
			resc_y, thumbnailCaptureCapability.width,
			resc_u, thumbnailCaptureCapability.width / 2,
			resc_v, thumbnailCaptureCapability.width / 2,
			thumbnailCaptureCapability.width, thumbnailCaptureCapability.height,
			libyuv::kFilterNone
		);
		//Pass it
		thumbnailCapture->IncomingFrame(resc_y, donwscaledSize, thumbnailCaptureCapability);
		//Free downscaled version
		free(downscaled);
	}

	//Increase number of pictures
	picId++;
*/
}

void MediaRoomBase::OnAudioFrame(void* frame)
{
	if (!frame)
		return;

	audio_data* pFrame = (audio_data*)frame;

	//Push it to the device
	m_AudioDeviceModule.onIncomingData(pFrame->data[0], pFrame->frames);
}

// WebRTCPeerNotifier ===============================================================================
bool MediaRoomBase::OnSdp(const std::string& peerID, const std::string& sType, const std::string& sText)
{
	ELOGD << "OnSdp :" << peerID << "," << sType << ", " << sText << endl;
	//ACTION : SDP Exchange
	m_pSignalClient->SdpExchange(m_authKey, peerID, sType, sText);

	return true;
}

bool MediaRoomBase::OnSdpFailed(const std::string& peerID)
{
	ELOGE << "sdp exchange:" << peerID << "failed" << endl;
	CallOnError(DOMAIN_MEDIAROOM, URI_PEER_CONNECTION, eErrorCode::PC_SetSDPFailed, "OnSdpFailed");

	return true;
}

bool MediaRoomBase::OnIce(const std::string& peerID, const std::string& sCandidate, int nSdpMLineIndex, const std::string& sSdpMid)
{
	ELOGD << "OnIce :" << peerID << "," << sCandidate << endl;
	//ACTION : ice candidate
	m_pSignalClient->IceCandidate(peerID, sCandidate, nSdpMLineIndex, sSdpMid);
	return true;
}

bool MediaRoomBase::OnIceConnectionChange(const std::string & peerID, int iceState)
{
	webrtc::PeerConnectionInterface::IceConnectionState _state = (webrtc::PeerConnectionInterface::IceConnectionState)iceState;

	elogi("MediaRoomBase::OnIceConnectionChange] iceState(%d) peerID(%s)", iceState, peerID.c_str());

	switch (_state)
	{
	case webrtc::PeerConnectionInterface::kIceConnectionNew:
		break;
	case webrtc::PeerConnectionInterface::kIceConnectionChecking:
		break;
	case webrtc::PeerConnectionInterface::kIceConnectionConnected:
		{
			ELOGD << "ice candidate :" << peerID << "connected" << endl;

			// set video max bitrates
			PeerConnection* peer = this->FindPeerConnection(peerID, false);
			if (peer==nullptr)
				ELOGE << "ice candidate :" << peerID << "failed FindPeerConnection" << endl;
			else
				peer->SetMaxBitRates(m_nMaxVideoBitrates);
		}
		break;
	case webrtc::PeerConnectionInterface::kIceConnectionCompleted:
		break;
	case webrtc::PeerConnectionInterface::kIceConnectionFailed:
		{
			ELOGE << "ice candidate :" << peerID << "failed" << endl;
			CallOnError(DOMAIN_MEDIAROOM, URI_PEER_CONNECTION, eErrorCode::PC_ConnectionFailed, "OnIceFailed");
			// leave room
		}
		break;
	case webrtc::PeerConnectionInterface::kIceConnectionDisconnected:
		{
			ELOGE << "ice candidate :" << peerID << "connection disconnected" << endl;
			CallOnError(DOMAIN_MEDIAROOM, URI_PEER_CONNECTION, eErrorCode::PC_ConnectionDisconnected, "OnIceDisconnected");
		}
		break;
	case webrtc::PeerConnectionInterface::kIceConnectionClosed:
		{
			ELOGE << "ice candidate :" << peerID << "connection closed" << endl;
			CallOnError(DOMAIN_MEDIAROOM, URI_PEER_CONNECTION, eErrorCode::PC_ConnectionClosed, "OnIceClosed");
			// leave room
		}
		break;
	case webrtc::PeerConnectionInterface::kIceConnectionMax:
		break;
	default:
		break;
	}

	m_pSignalClient->SendIceConnectionState(peerID, iceState);
	return false;
}

bool MediaRoomBase::OnStats(const std::string& peerID, void* data1, void* data2)
{
	std::lock_guard<std::mutex> guard(m_statInfoMutex);
	if(CommunityLiveCoreConfig::USE_STAT_INFO) {
		m_mediaStatInfo = *static_cast<MediaStatInfo*>(data1);
	}
	if(CommunityLiveCoreConfig::USE_STAT_INFO_DETAIL) {
		m_mediaStatDetailInfo = *static_cast<MediaStatDetailInfo*>(data2);
	}
	return true;
}
//===================================================================================================

void MediaRoomBase::RestartMediaRoom(const string& authKey)
{
	RemoveAllPeerConnection();

	m_authKey = authKey;
	m_bStarted = false;
	startMediaRoom();
}
