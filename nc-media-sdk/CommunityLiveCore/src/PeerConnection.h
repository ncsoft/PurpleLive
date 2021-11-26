#pragma once

#include "api/peer_connection_interface.h"
#include "WebRTCObservers.h"
#include <string>
#include <vector>

using namespace std;

//const string kPeerConnectionClientErrorDomain = "PeerConnectionClient";

enum PeerConnectionMediaType 
{
    //Voip
    PeerConnection_Voip,
    //Video
    PeerConnection_Video,
};

class PeerConnection :	public webrtc::PeerConnectionObserver,
						public webrtc::DataChannelObserver
{
public:
	PeerConnection(const string& peerID, PeerConnectionMediaType type, webrtc::AudioSourceInterface& audio, webrtc::VideoTrackSourceInterface& video, const vector<IceServerInfo>* pIceServerList = nullptr);
	virtual ~PeerConnection();

	void	StartSignaling(const string iceSever);
	
	void	SetMicEnable(bool enable);
	void	SetSpeakerEnable(bool enable);
	void	SetMaxBitRates(int max);
	bool	GetEnableStatObserver();
	void	SetEnableStatObserver(bool enable);

	// instruction event ====================================================================
	bool	StartOffer();
	bool	StartAnswer(const string& sdp);
	bool	StartAccept(const string& sdp);
	bool	StartIceAdd(const std::string& candidate, const std::string& sdpMid, int sdpMLineIndex);
//	bool	StartRejoin(const string& authKey);
	//=======================================================================================

	// PeerConnectionObserver ===============================================================
	virtual void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override;
	virtual void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;
	virtual void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;
	virtual void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override;
	virtual void OnRenegotiationNeeded() override;
	virtual void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override;
	virtual void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override;
	virtual void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
	virtual void OnIceSelectedCandidatePairChanged(const cricket::CandidatePairChangeEvent& event) override;
	//=======================================================================================

	// DataChannelObserver ==================================================================
	virtual void OnStateChange() override {}
	virtual void OnMessage(const webrtc::DataBuffer& buffer) override {}
//	virtual void OnBufferedAmountChange(uint64_t previous_amount) override {}

	rtc::scoped_refptr<webrtc::PeerConnectionInterface>& GetPeerConnection() { return m_peer; }
	rtc::scoped_refptr<WebRTC_SDP_Observer>& GetSdpObserver() { return m_sdpObserver; }
	rtc::scoped_refptr<WebRTC_Stat_Observer>& GetStatObserver() { return m_statObserver; }

protected:
	void	Close();

	string						m_peerID;
	PeerConnectionMediaType		m_mediaType;
	bool						m_bConnected;
	bool						m_bUseMic;
	bool						m_bUseSpeaker;
	int							m_nMaxBitrate;
	bool						m_bReadyStatObserver;

	rtc::scoped_refptr<webrtc::AudioTrackInterface>	m_localAudioTrack;
	rtc::scoped_refptr<webrtc::VideoTrackInterface> m_localVideo_track;
	webrtc::MediaStreamInterface*				m_pRemoteStream = nullptr;

	rtc::scoped_refptr<webrtc::PeerConnectionInterface> m_peer;
	rtc::scoped_refptr<WebRTC_SDP_Observer>				m_sdpObserver;
	rtc::scoped_refptr<WebRTC_Stat_Observer>			m_statObserver;
};
