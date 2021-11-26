#include "CommunityLiveCoreConfig.h"
#include "PeerConnection.h"
#include "WebRTCManager.h"
#include "MediaRoom.h"
#include "MediaRoomManager.h"
#include "easylogging++.h"

const string s_StreamID = "community_live_stream_id";
const string s_AudioLabel = "community_live_audio_label";
const string s_VideoLabel = "community_live_video_label";

PeerConnection::PeerConnection(	const string& peerID, PeerConnectionMediaType type, 
	webrtc::AudioSourceInterface& audio, webrtc::VideoTrackSourceInterface& video, 
	const vector<IceServerInfo>* pIceServerList/*= nullptr*/)
{
	m_peerID = peerID;
	m_mediaType = type;
	m_nMaxBitrate = -1;

	webrtc::PeerConnectionInterface::RTCConfiguration config;
	config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
	config.enable_dtls_srtp = true;

	if (pIceServerList)
	{
		for (const auto& ice : *pIceServerList)
		{
			bool is_turn = (ice.url.find("turn") != string::npos);
			webrtc::PeerConnectionInterface::IceServer svr;
			svr.uri = ice.url;
			if (is_turn)
			{
				svr.username = ice.username;
				svr.password = ice.password;
			}
			config.servers.push_back(svr);
		}
	}

	m_peer = g_WebRTC_PCF.CreatePeerConnection(config, nullptr, nullptr, this);
	// stat정보 얻기(TODO : 필요시 활성화할것)
	//m_peer->GetStats(m_statObserver);

	if (!m_peer)
	{
		MediaRoom* pMediaRoom = MediaRoomManager::Instance()->GetCurrentMediaRoom();
		if(pMediaRoom)
			pMediaRoom->CallOnError(DOMAIN_MEDIAROOM, URI_PEER_CONNECTION, PC_CreateFailed, "CreatePeerConnection Failed");
//		assert(m_peer);
		return;
	}

	MediaRoom* pMediaRoom = MediaRoomManager::Instance()->GetCurrentMediaRoom();

	// Audio Track 등록
	// TODO : iOS는 AudioSource, VideoSource를 WebRTCManager에서 생성
	m_localAudioTrack = g_WebRTC_PCF.CreateAudioTrack(s_AudioLabel, &audio);
	auto r = m_peer->AddTrack(m_localAudioTrack, { s_StreamID });
	if (!r.ok())
	{
		if (pMediaRoom)
			pMediaRoom->CallOnError(DOMAIN_MEDIAROOM, URI_PEER_CONNECTION, PC_CreateFailed, "Failed to add audio track to PeerConnection");
//		assert(r.ok());
		return;	//생성자에서 return??
	}

	if (m_mediaType == PeerConnection_Video)
	{
		// Video Track 등록
		m_localVideo_track = g_WebRTC_PCF.CreateVideoTrack(s_VideoLabel, &video);
		auto r = m_peer->AddTrack(m_localVideo_track, { s_StreamID });
		if (!r.ok())
		{
			if (pMediaRoom)
				pMediaRoom->CallOnError(DOMAIN_MEDIAROOM, URI_PEER_CONNECTION, PC_CreateFailed, "Failed to add video track to PeerConnection");
			return;	//생성자에서 return??
		}
	}

	// SDP Observer 등록
	m_sdpObserver = new rtc::RefCountedObject<WebRTC_SDP_Observer>();
	if (!m_sdpObserver)
	{
		if (pMediaRoom)
			pMediaRoom->CallOnError(DOMAIN_MEDIAROOM, URI_PEER_CONNECTION, PC_CreateFailed, "SDP Overserver alloc Failed");
//		assert(m_sdpObserver.get());
		return; //생성자에서 return??
	}
	m_sdpObserver->Setup(*m_peer, pMediaRoom, m_peerID);

	// Stat Observer 등록
	if (CommunityLiveCoreConfig::USE_STAT_INFO) 
	{
		if (pMediaRoom)
		{
			int64_t result;
			pMediaRoom->ProcessCommand(MediaRoomCommand::GetEnableStatObserver, 0, result);
			if (result)
			{
				SetEnableStatObserver(true);
			}
		}
	}
}

PeerConnection::~PeerConnection()
{
	Close();
	m_peer = nullptr;
	m_sdpObserver = nullptr;
	m_statObserver = nullptr;
	RTC_DCHECK(!m_peer);
}

void PeerConnection::SetMicEnable(bool enable)
{
	m_bUseMic = enable;

	m_localAudioTrack->set_enabled(enable);
}

void PeerConnection::SetSpeakerEnable(bool enable)
{
	m_bUseSpeaker = enable;

	if (!m_pRemoteStream)
		return;

	webrtc::AudioTrackVector tracks = m_pRemoteStream->GetAudioTracks();
	if (tracks.empty())
		return;

	webrtc::AudioTrackInterface* audio_track = tracks[0];

	std::string id = audio_track->id();

	for (auto& track : tracks)
	{
		track->set_enabled(m_bUseSpeaker);
	}
}

void PeerConnection::SetMaxBitRates(int max)
{
	if(m_nMaxBitrate==max)
		return;

	m_nMaxBitrate = max;
	ELOGD << format_string("[PeerConnection::SetMaxBitRates] MaxBitrate(%d)", m_nMaxBitrate);

	bool result = true;
	std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> senders = m_peer->GetSenders();
	for (auto sender : senders)
	{
		if (sender->media_type() != cricket::MEDIA_TYPE_VIDEO)
			continue;

		auto parameters = sender.get()->GetParameters();
		for (auto& encoding : parameters.encodings) 
		{
			encoding.max_bitrate_bps = absl::optional<int>{ max };
		}

		webrtc::RTCError error = sender.get()->SetParameters(parameters);
		if (!error.ok()) 
		{
			MediaRoom* pMediaRoom = MediaRoomManager::Instance()->GetCurrentMediaRoom();
			if (pMediaRoom)
			{
				pMediaRoom->CallOnError(DOMAIN_MEDIAROOM, URI_PEER_CONNECTION, eErrorCode::PC_SetSDPFailed, "SetBitRates Failed");
				ELOGD << format_string("[PeerConnection::SetMaxBitrate] RtpSender.SetParameters error sender(0x%x) msg(%s)", sender, error.message());
			}
		}
	}
//	return result;
}

bool PeerConnection::GetEnableStatObserver()
{
	return m_bReadyStatObserver;
}

void PeerConnection::SetEnableStatObserver(bool enable)
{
	if (enable)
	{
		MediaRoom* pMediaRoom = MediaRoomManager::Instance()->GetCurrentMediaRoom();
		if (pMediaRoom && m_statObserver==nullptr)
		{
			m_statObserver = new rtc::RefCountedObject<WebRTC_Stat_Observer>();
			if (!m_statObserver)
			{
				ELOGE << "Stat Observer alloc fail";
				assert(m_statObserver.get());
				return;
			}
			m_statObserver->Setup(*m_peer, pMediaRoom, m_peerID);
		}
		m_bReadyStatObserver = true;
	}
	else
	{
		m_bReadyStatObserver = false;
		if (m_statObserver)
		{
			delete m_statObserver;
			m_statObserver = nullptr;
		}
	}
}

// instruction event ====================================================================
bool PeerConnection::StartOffer()
{
	ELOGD << "=======================================";
	ELOGD << " CreateOffer :";
	ELOGD << "=======================================";
	m_peer->CreateOffer(m_sdpObserver, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());

	return true;
}

bool PeerConnection::StartAnswer(const string& sdp)
{
	ELOGD << "=======================================";
	ELOGD << " CreateAnswer :";
	ELOGD << "=======================================";

	webrtc::SdpParseError error;
	webrtc::SessionDescriptionInterface* pSdp = 
		webrtc::CreateSessionDescription(webrtc::SdpType::kOffer, sdp, &error).release();
	if (!pSdp)
	{
		MediaRoom* pMediaRoom = MediaRoomManager::Instance()->GetCurrentMediaRoom();
		if (pMediaRoom)
			pMediaRoom->CallOnError(DOMAIN_MEDIAROOM, URI_PEER_CONNECTION, eErrorCode::PC_CreateSDPFailed, "CreateSessionDescription Failed(StartAnswer)");
//		ELOGE << "failed to set offer SDP: " << error.description;
		return false;
	}

	ELOGD << "=======================================";
	ELOGD << " Received offer SDP :";
	ELOGD << sdp;
	ELOGD << "=======================================";

	m_peer->SetRemoteDescription(new rtc::RefCountedObject<WebRTC_SetSDP_Observer>(), pSdp);
	m_peer->CreateAnswer(m_sdpObserver, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());

	return true;
}

bool PeerConnection::StartAccept(const string& sdp)
{
	ELOGD << "=======================================";
	ELOGD << " StartAccept :";
	ELOGD << "=======================================";

	webrtc::SdpParseError error;
	webrtc::SessionDescriptionInterface* pSdp =
		webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, sdp, &error).release();
	if (!pSdp)
	{
		MediaRoom* pMediaRoom = MediaRoomManager::Instance()->GetCurrentMediaRoom();
		if (pMediaRoom)
			pMediaRoom->CallOnError(DOMAIN_MEDIAROOM, URI_PEER_CONNECTION, eErrorCode::PC_CreateSDPFailed, "CreateSessionDescription Failed(StartAccept)");
//		ELOGE << "failed to set answer SDP: " << error.description;
		return false;
	}

	ELOGD << "=======================================";
	ELOGD << " Received answer SDP :";
	ELOGD << sdp;
	ELOGD << "=======================================";

	MediaRoom* pMediaRoom = MediaRoomManager::Instance()->GetCurrentMediaRoom();

	if (pMediaRoom) {
		int64_t bitrates;
		pMediaRoom->ProcessCommand(MediaRoomCommand::GetMaxVideoBitrates, 0, bitrates);
		SetMaxBitRates((int)bitrates);
	}

	m_peer->SetRemoteDescription(new rtc::RefCountedObject<WebRTC_SetSDP_Observer>(), pSdp);

	if (pMediaRoom)
		pMediaRoom->CallOnNotify(DOMAIN_MEDIAROOM, "SDP_Accept", "");

	return true;
}

bool PeerConnection::StartIceAdd(const std::string& candidate, const std::string& sdpMid, int sdpMLineIndex)
{
	ELOGD << "=======================================";
	ELOGD << " StartIceAdd :";
	ELOGD << "=======================================";

	webrtc::SdpParseError error;
	std::unique_ptr<webrtc::IceCandidateInterface> _candidate(webrtc::CreateIceCandidate(sdpMid, sdpMLineIndex, candidate, &error));
	if (!_candidate.get()) 
	{
		ELOGE << "failed to parse ICE candidate: " << error.description;
		return false;
	}
	if (!m_peer->AddIceCandidate(_candidate.get())) 
	{
		ELOGE << "failed to set ICE candidate";
		return false;
	}

	ELOGD << "=======================================";
	ELOGD << " Received sSdpMid :" << sdpMid;
	ELOGD << " Received sdpMLineIndex :" << sdpMLineIndex;
	ELOGD << " Received candidate :" << candidate;
	ELOGD << "=======================================";
	
	return true;
}
/*
bool PeerConnection::StartRejoin(const string& authKey)
{

}
*/
//=======================================================================================

// PeerConnectionObserver ===============================================================
void PeerConnection::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state)
{
}

void PeerConnection::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
{
	m_pRemoteStream = stream;
}

void PeerConnection::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
{
	m_pRemoteStream = nullptr;
}

void PeerConnection::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel)
{
}

void PeerConnection::OnRenegotiationNeeded()
{
}

void PeerConnection::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state)
{
	MediaRoom* pMediaRoom = MediaRoomManager::Instance()->GetCurrentMediaRoom();
	if (pMediaRoom)
	{
		pMediaRoom->OnIceConnectionChange(m_peerID, (int)new_state);
	}
}

void PeerConnection::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state)
{
}

void PeerConnection::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
{
	MediaRoom* pMediaRoom = MediaRoomManager::Instance()->GetCurrentMediaRoom();
	if (pMediaRoom)
	{
		std::string cand;
		candidate->ToString(&cand);
		pMediaRoom->OnIce(m_peerID, cand, candidate->sdp_mline_index(), candidate->sdp_mid());
	}
}

void PeerConnection::OnIceSelectedCandidatePairChanged(const cricket::CandidatePairChangeEvent& event) 
{
	bool is_relay = event.selected_candidate_pair.local.type()=="relay";
	string url = event.selected_candidate_pair.local.url();
	string protocol = event.selected_candidate_pair.local.protocol();
	string relay_protocol = event.selected_candidate_pair.local.relay_protocol();
	ELOGI << format_string("[PeerConnection::OnIceSelectedCandidatePairChanged] RELAY(%d) protocol(%s) relay_protocol(%s) url(%s)", is_relay, protocol.c_str(), relay_protocol.c_str(), url.c_str());
}

//=======================================================================================

void PeerConnection::Close()
{
	if(m_peer) {
		m_peer->Close();
	}
}