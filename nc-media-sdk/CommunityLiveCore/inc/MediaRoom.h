#pragma once
/*	
	하나의 미디어 세션에 대한 관리
	미디어 세션 연관된 PeerConnection 관리자
*/
#include "common.h"
#include "CommunityLiveError.h"
#include <string>
#include <map>
#include <vector>

using namespace std;

const string kMediaRoomErrorDomain = "MediaRoom";
const int DEFAULT_KEEP_ALIVE_INTERVAL = 30;

class PeerConnection;
class SignalClient;
struct WebRTCMediaSource;
struct IceServerInfo;

typedef map<string, PeerConnection*>			MAP_PEERCONNECTION;
typedef map<string, PeerConnection*>::iterator	MAP_PEERCONNECTION_ITR;

enum MediaRoomType 
{
    MediaRoom_VoiceChat,
    MediaRoom_ScreenShare,
    MediaRoom_Watching,
    MediaRoom_Camera,
	MediaRoom_LiveStudio,
};

enum MediaRoomCommand
{
	MRC_Unknown = 0,
	GetEnableStatObserver,
	SetEnableStatObserver,
	SetEnableStatLog,
	GetSentBytes,
	GetFrameSent,
	GetFrameDropped,
	GetMediaStatDetail,
	GetFrameRate,
	SetFrameRate,
	GetMaxVideoBitrates,
	SetMaxVideoBitrates,
	SetEncoderQuilityPreset,	// 0: speed, 1: balance, 2: quility (default: speed)
	SetEncoderRateControl,		// 0: VBR,   1: CBR                 (default: VBR)
	SetEnableEncoderLogFile,
	SetIntervalEncoderLogFile,
};

#ifndef _EncoderQualityPreset
#define _EncoderQualityPreset
	enum EncoderQualityPreset {
		Speed = 0,
		Balanced,
		Quality,
	};
#endif //_EncoderQualityPreset

#ifndef _EncoderRateControl
#define _EncoderRateControl
	enum class EncoderRateControl: uint8_t {
		VBR = 0,
		CBR,
	};
#endif //_EncoderRateControl


// -----------------------

typedef void(*MediaRoomDelegateFuncPtr)(void* owner, int32_t, const char*, const char*, int32_t, const char*);

class WebRTCPeerNotifier 
{
public:
	virtual bool OnSdp(const std::string& peerID, const std::string& sType, const std::string& sText) = 0;
	virtual bool OnSdpFailed(const std::string& peerID) = 0;
	virtual bool OnIce(const std::string& peerID, const std::string& sCandidate, int nSdpMLineIndex, const std::string& sSdpMid) = 0;
	virtual bool OnIceConnectionChange(const std::string& peerID, int iceState) = 0;
	virtual bool OnStats(const std::string& peerID, void* data1, void* data2) = 0;
};

class COMMUNITY_LIVE_CORE_API MediaRoom : public WebRTCPeerNotifier
{
public:
	MediaRoom() : m_pMediaRoomDelegate(nullptr) {}
	virtual ~MediaRoom() {}
	virtual bool	startMediaRoom()=0;
	virtual void	stopMediaRoom()=0;
	virtual void	Init(const string& url, const string& authkey, VideoEncodingType encodeType, int framerate)=0;

	virtual bool	ReconnectSignal() = 0;
	virtual void	RestartMediaRoom(const string& authKey)=0;
	virtual PeerConnection*	FindPeerConnection(const string& peerID, bool bCreateIfNotExist, const vector<IceServerInfo>* pIceServerList = nullptr)=0;
	virtual void			RemovePeerConnection(const string& peerID)=0;
	virtual void			RemoveAllPeerConnection()=0;

	virtual int64_t	ProcessCommand(int32_t command, int32_t type) = 0;
	virtual bool	ProcessCommand(int32_t command, int32_t type, int64_t& result) = 0;

	void			SetDelegate(MediaRoomDelegateFuncPtr pDelegate, void* pOwner) { m_pDelegateOwner = pOwner;  m_pMediaRoomDelegate = pDelegate; }

	void			CallOnStarted();
	void			CallOnStopped();
	void			CallOnNotify(string domain, string uri, string msg);
	void			CallOnError(string domain, string uri, int32_t error, string msg);

	virtual void	SetMicEnable(bool enable)		{ m_bUseMic = enable; }
	virtual void	SetSpeakerEnable(bool enable)	{ m_bUseSpeaker = enable; }

	virtual void	SetConnectionInfo(const char* url, const char* authKey) = 0;

	virtual void	OnVideoFrame(void* frame, uint32_t width, uint32_t height, bool rgb=false) = 0;
	virtual void	OnAudioFrame(void* frame) = 0;

	MediaRoomType	GetType() { return m_type; }

	// WebRTCPeerNotifier ===============================================================================
	virtual bool OnSdp(const std::string& peerID, const std::string& sType, const std::string& sText)=0;
	virtual bool OnSdpFailed(const std::string& peerID)=0;
	virtual bool OnIce(const std::string& peerID, const std::string& sCandidate, int nSdpMLineIndex, const std::string& sSdpMid)=0;
	virtual bool OnIceConnectionChange(const std::string& peerID, int iceState) = 0;
	virtual bool OnStats(const std::string& peerID, void* data1, void* data2)=0;

protected:
	MediaRoomDelegateFuncPtr	m_pMediaRoomDelegate;
	void*						m_pDelegateOwner;
	MediaRoomType				m_type;
	VideoEncodingType		m_encodeType;

	bool			m_bUseMic;
	bool			m_bUseSpeaker;
};