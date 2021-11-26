#pragma once
/*
	웹소켓 클래스
	시그널 서버와 통신
*/

#include "../../CLCWebsocketClient/inc/CLCWebsocketClient.h"

#include <string>
#include "TransactionId.hpp"
#include "StructureDefines.h"
#include "json/json.h"
#include <windows.h>
#include <mmsystem.h>
#pragma comment (lib, "Winmm.lib")

#define DEFAULT_SIGNAL_KEEPALIVE_INTERVAL 30000

using namespace std;

const string kSignalClientErrorDomain = "SignalClient";

enum MediaStatus {
	Inactive,
	SendReceive,
	SendOnly,
	ReceiveOnly,
};

class MediaRoom;
struct IceServerInfo;

class SignalClient : public CLCWebsocketClient
{
public:
	SignalClient(MediaRoom* pOwner);
	~SignalClient();

	bool	JoinRoom(const string& signalServerURL, const string& authKey);
	void	LeaveRoom(const string& authKey);
	void	SdpExchange(const string authKey, const string peerID, const string type, const string sdp);
	void	IceCandidate(const string peerID, const string candidate, const int sdpMLineIndex, const string& sdpMid);
	void	SendIceConnectionState(const string& peerID, int state);
	void	Close();

	bool	_Connect(const string& url, const string& authKey); // 소켓 초기화 및 Signal 서버에 연결
protected:
	void	SendMsg(const string& msg);

	virtual void	OnClose(const string& reason, int errorCode) override;
	virtual int		OnMessage(const string& strMsg) override;
	int		_OnMessage_InstructionEvent(const Json::Value& jMsg);
	int		_OnMessage_InfoEvent(const Json::Value& jMsg);

	void			SendKeepAlive();
	static void CALLBACK KeepAliveProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

	void	StartKeepAliveTimer();
	void	StopKeepAliveTimer();

	string	m_signalServerURL;
	string	m_authKey;

	CTransactionId		m_TransactionIDGenerator;	// transactionID를 생성하는 난수발생기

	short				m_nKeepAliveIntervalms;
	UINT				m_nTimerID;

	PeerInfo			m_stPeerInfo;
	MediaRoom*			m_pOwner;	// 웹소켓을 소유한 MediaRoom 객체 포인터
};
