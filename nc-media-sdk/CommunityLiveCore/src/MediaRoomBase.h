#pragma once
/*	
	하나의 미디어 세션에 대한 관리
	미디어 세션 연관된 PeerConnection 관리자
*/
#include "CommunityLiveCoreConfig.h"
#include "MediaRoom.h"
#include "CommunityLiveError.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_defines.h"
#include "modules/video_capture/video_capture_factory.h"
#include "AudioDeviceModuleWrapper.h"

#include <string>
#include <map>
#include <vector>
#include <mutex>

#define DEFAULT_MAX_VIDEO_BITRATE	2500000

using namespace std;

// for OBS Studio -------
#define MAX_AV_PLANES 8
struct video_data {
	uint8_t           *data[MAX_AV_PLANES];
	uint32_t          linesize[MAX_AV_PLANES];
	uint64_t          timestamp;
};

struct audio_data {
	uint8_t             *data[MAX_AV_PLANES];
	uint32_t            frames;
	uint64_t            timestamp;
};

struct MediaStatInfo {
private:
	bool		enable_observer = false;
	bool		enable_log = false;

public:
	uint64_t	audio_bytes_sent = 0;
	uint64_t	video_bytes_sent = 0;
	uint32_t	packets_sent = 0;
	double		target_bitrate = 0;
	uint32_t	frames_encoded = 0;
	uint32_t	frames_dropped = 0;
	uint32_t	frames_sent = 0;

	MediaStatInfo() { initialize(); }

	MediaStatInfo& operator=(const MediaStatInfo& msi) {
		audio_bytes_sent = msi.audio_bytes_sent;
		video_bytes_sent = msi.video_bytes_sent;
		packets_sent = msi.packets_sent;
		target_bitrate = msi.target_bitrate;
		frames_encoded = msi.frames_encoded;
		frames_dropped = msi.frames_dropped;
		frames_sent = msi.frames_sent;
		return *this;
	}

	void initialize() {
		audio_bytes_sent = 0;
		video_bytes_sent = 0;
		packets_sent = 0;
		target_bitrate = 0.;
		frames_encoded = 0;
		frames_dropped = 0;
		frames_sent = 0;
	}

	bool getEnableObserver() { return enable_observer; }
	void setEnableObserver(bool value) { enable_observer = value; }

	bool getEnableLog() { return enable_log; }
	void setEnableLog(bool value) { enable_log = value; }
};

struct MediaStatDetailInfo {
	///< RTCIceCandidatePairStats
	uint64_t timestamp_us = 0;
	uint64_t bytes_sent = 0;
	uint64_t bytes_received = 0;
	double total_round_trip_time = 0;
	double current_round_trip_time = 0;
	double available_outgoing_bitrate = 0;
	double available_incoming_bitrate = 0;
	uint64_t requests_received = 0;
	uint64_t requests_sent = 0;
	uint64_t responses_received = 0;
	uint64_t responses_sent = 0;
	uint64_t retransmissions_received = 0;
	uint64_t retransmissions_sent = 0;

	///< RTCMediaStreamTrackStats (common)
	std::string kind;

	///< RTCMediaStreamTrackStats (audio)
	struct AudioStreamTrackStatInfo {
		double audio_level = 0;
		double total_audio_energy = 0;
		double echo_return_loss = 0;
		double echo_return_loss_enhancement = 0;
		uint64_t total_samples_received = 0;
		double total_samples_duration = 0;
		uint64_t concealed_samples = 0;
		uint64_t concealment_events = 0;
	};
	AudioStreamTrackStatInfo astsi;

	///< RTCMediaStreamTrackStats (video)
	struct VideoStreamTrackStatInfo {
		double jitter_buffer_delay = 0;
		uint32_t frame_width = 0;
		uint32_t frame_height = 0;
		uint32_t frames_sent = 0;
		uint32_t huge_frames_sent = 0;
		uint32_t frames_received = 0;
		uint32_t frames_decoded = 0;
		uint32_t frames_dropped = 0;
		uint32_t frames_corrupted = 0;
		uint32_t partial_frames_lost = 0;
		uint32_t full_frames_lost = 0;
		double frames_per_second = 0;

		uint32_t freeze_count = 0;
		uint32_t pause_count = 0;
		double total_freezes_duration = 0;
		double total_pauses_duration = 0;
		double total_frames_duration = 0;
		double sum_squared_frame_durations = 0;
	};
	VideoStreamTrackStatInfo vstsi;

	///< RTCRTPStreamStats (video)
	struct VideoStreamStatInfo {
		uint32_t ssrc = 0;
		bool is_remote = false;
		std::string track_id;
		std::string transport_id;
		std::string codec_id;
		uint32_t fir_count = 0;
		uint32_t pli_count = 0;
		uint32_t nack_count = 0;
		uint32_t sli_count = 0;
		uint64_t qp_sum = 0;
	};
	VideoStreamStatInfo vssi;

	///< RTCOutboundRTPStreamStats (video)
	struct MediaOutboundStatInfo {
		std::string media_source_id;
		std::string remote_id;
		std::string rid;

		uint32_t packets_sent = 0;
		uint64_t retransmitted_packets_sent = 0;

		//uint64_t bytes_sent = 0;
		uint64_t audio_bytes_sent = 0;
		uint64_t video_bytes_sent = 0;
		uint64_t header_bytes_sent = 0;
		uint64_t retransmitted_bytes_sent = 0;

		double target_bitrate = 0;
		uint32_t frames_encoded = 0;
		uint32_t key_frames_encoded = 0;
		double total_encode_time = 0;
		uint64_t total_encoded_bytes_target = 0;
		
		uint32_t frame_width = 0;
		uint32_t frame_height = 0;
		double frames_per_second = 0;
		uint32_t frames_sent = 0;
		uint32_t huge_frames_sent = 0;
		double total_packet_send_delay = 0;
		
		std::string quality_limitation_reason;
		uint32_t quality_limitation_resolution_changes;
		std::string content_type;
		std::string encoder_implementation;
	};
	MediaOutboundStatInfo mosi;
};

struct IceServerInfo
{
	std::string url;
	std::string username;
	std::string password;	
};

class MediaRoomBase : public MediaRoom//, public cricket::WebRtcVcmFactoryInterface
{
public:
	MediaRoomBase();
	virtual ~MediaRoomBase();

	virtual bool	startMediaRoom();
	virtual void	stopMediaRoom();

	virtual void	Init(const string& url, const string& authkey, VideoEncodingType encodeType, int framerate);

	virtual int64_t ProcessCommand(int32_t command, int32_t type);
	virtual bool	ProcessCommand(int32_t command, int32_t type, int64_t& result);

	virtual void	SetMicEnable(bool enable_observer) override;
	virtual void	SetSpeakerEnable(bool enable_observer) override;
	virtual void	SetConnectionInfo(const char* url, const char* authKey) override { m_signalServerUrl = url; m_authKey = authKey; }

	// signal server related
	virtual bool	ReconnectSignal() override;

	// PeerConnection related
	virtual PeerConnection*	FindPeerConnection(const string& peerID, bool bCreateIfNotExist, const vector<IceServerInfo>* pIceServerList=nullptr) override;
	virtual void			RemovePeerConnection(const string& peerID) override;
	virtual void			RemoveAllPeerConnection() override;

	virtual void	RestartMediaRoom(const string& authKey);

	virtual void	OnVideoFrame(void* frame, uint32_t width, uint32_t height, bool rgb = false) override;
	virtual void	OnAudioFrame(void* frame) override;

	// WebRTCPeerNotifier ===============================================================================
	virtual bool OnSdp(const std::string& peerID, const std::string& sType, const std::string& sText) override;
	virtual bool OnSdpFailed(const std::string& peerID) override;
	virtual bool OnIce(const std::string& peerID, const std::string& sCandidate, int nSdpMLineIndex, const std::string& sSdpMid) override;
	virtual bool OnIceConnectionChange(const std::string& peerID, int iceState) override;
	virtual bool OnStats(const std::string& peerID, void* data1, void* data2) override;
	//===================================================================================================

	/*
	// WebRtcVcmFactoryInterface =======================================================
	rtc::scoped_refptr<webrtc::VideoCaptureModule> GetCaptureModule()
	{
		return m_pVideoCaptureModule;
	}

	virtual rtc::scoped_refptr<webrtc::VideoCaptureModule> Create(const char*)
	{
		return m_pVideoCaptureModule;
	}
	virtual webrtc::VideoCaptureModule::DeviceInfo* CreateDeviceInfo()
	{
		return webrtc::VideoCaptureFactory::CreateDeviceInfo();
	}
	virtual void DestroyDeviceInfo(webrtc::VideoCaptureModule::DeviceInfo* info)
	{
		delete(info);
	}
	//===================================================================================
	*/
protected:
	virtual void	Destroy();
	bool			GetEnableStatObserver();
	void			SetEnableStatObserver(bool enable);
	void			SetEnableStatLog(bool enable);
	void			UpdateStatInfo();
	void			StartStatThread();
	void			StopStatThread();
	
	bool			m_bStarted;
	bool			m_bReconnection;

	string			m_signalServerUrl;
	string			m_authKey;

	int				m_nKeepAliveInterval;

	MAP_PEERCONNECTION	m_mapPeerConnection;

	SignalClient*		m_pSignalClient;

	WebRTCMediaSource*	m_pMediaSource;

	//Audio Wrapper
	AudioDeviceModuleWrapper				m_AudioDeviceModule;
	//Video Wrappers
	webrtc::VideoCaptureCapability			m_VideoCaptureCapability;

	MediaStatInfo		m_mediaStatInfo;
	MediaStatDetailInfo	m_mediaStatDetailInfo;
	std::mutex			m_statInfoMutex;
	std::thread			m_statThread;
	bool				m_bStatThreadStop;
	bool				m_bStatThreadStarted;
	bool				m_bStatBlocking;
	int					m_nMaxVideoBitrates;
	int					m_nFrameRate;
};
