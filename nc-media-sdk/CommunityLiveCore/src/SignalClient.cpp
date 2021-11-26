#include "SignalClient.h"
#include "CommunityLiveError.h"
#include "MediaRoom.h"
#include "PeerConnection.h"
#include "easylogging++.h"

#define	INVALID_PARAM 2

// utility function
#define PTRCHK_NULL(_ptr_, _errcode_)	\
	do {								\
		if(!(_ptr_))					\
		{								\
			ELOGE << "null " << #_ptr_;	\
			return _errcode_;			\
		}								\
	} while(0)

#define JCHK_NULL(_jv_) PTRCHK_NULL(_jv_, INVALID_PARAM)

#define JCHK_TYPE(_jv_, _typefn_)		\
	do {								\
		if(!(_jv_)._typefn_())			\
		{								\
			ELOGE << #_jv_ << " "		\
			<< #_typefn_ << "() false";	\
			return INVALID_PARAM;		\
		}								\
	} while(0)


#define JCHK(_jv_, _typefn_)		\
	do {							\
		JCHK_NULL(_jv_);			\
		JCHK_TYPE(_jv_, _typefn_);	\
	} while(0)


SignalClient::SignalClient(MediaRoom* pOwner)
{
	m_pOwner = pOwner;
	m_nKeepAliveIntervalms = DEFAULT_SIGNAL_KEEPALIVE_INTERVAL;
	m_nTimerID = 0;
}

SignalClient::~SignalClient()
{
	Close();
}

bool SignalClient::JoinRoom(const string& signalServerURL, const string& authKey)
{
	if (_Connect(signalServerURL, authKey) == false)
	{
		return false;
	}

	// send join msg
	Json::Value jMsg;

	jMsg["tid"] = m_TransactionIDGenerator.GenStr();
	jMsg["action"] = "join_stream";
	jMsg["auth_key"] = authKey;

	Json::StreamWriterBuilder writer;
	SendMsg(Json::writeString(writer, jMsg));

	return true;
}

void SignalClient::LeaveRoom(const string& authKey)
{
	// send leave msg
	Json::Value jMsg;

	jMsg["tid"] = m_TransactionIDGenerator.GenStr();
	jMsg["action"] = "leave_room";
	jMsg["auth_key"] = authKey;

	Json::StreamWriterBuilder writer;
	SendMsg(Json::writeString(writer, jMsg));
}

void SignalClient::SdpExchange(const string authKey, const string peerID, const string type, const string sdp)
{
	// send sdp exchange msg
	Json::Value jMsg;

	jMsg["tid"]				= m_TransactionIDGenerator.GenStr();
	jMsg["action"]			= "sdp_exchange";
	jMsg["auth_key"]		= authKey;
	jMsg["peer_id"]			= peerID;
	jMsg["jsep"]["type"]	= type;
	jMsg["jsep"]["sdp"]		= sdp;

	Json::StreamWriterBuilder writer;
	SendMsg(Json::writeString(writer, jMsg));
}

void SignalClient::IceCandidate(const string peerID, const string candidate, const int sdpMLineIndex, const string& sdpMid)
{
	Json::Value jMsg;

	jMsg["tid"] = m_TransactionIDGenerator.GenStr();
	jMsg["action"] = "ice_candidate";
	jMsg["auth_key"] = m_authKey;
	jMsg["peer_id"] = peerID;
	jMsg["ice_candidate"]["candidate"] = candidate;
	jMsg["ice_candidate"]["sdpMLineIndex"] = sdpMLineIndex;
	jMsg["ice_candidate"]["sdpMid"] = sdpMid;

	Json::StreamWriterBuilder writer;
	SendMsg(Json::writeString(writer, jMsg));
}

void SignalClient::SendIceConnectionState(const string & peerID, int state)
{
	webrtc::PeerConnectionInterface::IceConnectionState _state = (webrtc::PeerConnectionInterface::IceConnectionState)state;

	string strState = "";
	switch (_state)
	{
	case webrtc::PeerConnectionInterface::kIceConnectionNew:
		strState = "new";
		break;
	case webrtc::PeerConnectionInterface::kIceConnectionChecking:
		strState = "checking";
		break;
	case webrtc::PeerConnectionInterface::kIceConnectionConnected:
		strState = "connected";
		break;
	case webrtc::PeerConnectionInterface::kIceConnectionCompleted:
		strState = "completed";
		break;
	case webrtc::PeerConnectionInterface::kIceConnectionFailed:
		strState = "failed";
		break;
	case webrtc::PeerConnectionInterface::kIceConnectionDisconnected:
		strState = "disconnected";
		break;
	case webrtc::PeerConnectionInterface::kIceConnectionClosed:
		strState = "closed";
		break;
	case webrtc::PeerConnectionInterface::kIceConnectionMax:
	default:
		strState = "none";
		break;
	}
	Json::Value jMsg;

	jMsg["tid"] = m_TransactionIDGenerator.GenStr();
	jMsg["action"] = "trace";
	jMsg["auth_key"] = m_authKey;
	jMsg["peer_id"] = peerID;
	jMsg["version"] = "v1";
	jMsg["ice_connection_state"]["state"] = strState;

	Json::StreamWriterBuilder writer;
	SendMsg(Json::writeString(writer, jMsg));
}

void SignalClient::SendKeepAlive()
{
	// send keep alive msg
	Json::Value jMsg;

	jMsg["tid"] = m_TransactionIDGenerator.GenStr();
	jMsg["action"] = "keep_alive";
	jMsg["auth_key"] = m_authKey;

	Json::StreamWriterBuilder writer;
	SendMsg(Json::writeString(writer, jMsg));
}

bool SignalClient::_Connect(const string& url, const string& authKey)
{
	WSC_Result re = Connect(url, authKey);

	if (re != WSC_Result::WSC_OK)
		return false;

	// save info
	m_signalServerURL = url;
	m_authKey = authKey;

	StartKeepAliveTimer();

	return true;
}

void SignalClient::SendMsg(const string& msg)
{
	ELOGD << "################## send msg : " << msg;
	WSC_Result re = _SendMessage(msg);

	if (re != WSC_Result::WSC_OK)
		return;
}

void SignalClient::Close()
{
	// disconnect
	StopKeepAliveTimer();
}

void SignalClient::OnClose(const string & reason, int errorCode)
{
	if (errorCode != 0)
		m_pOwner->CallOnError(DOMAIN_SIGNAL_CLIENT, URI_SIGNAL_CLIENT_CONNECTION, errorCode, "");
	ELOGD << "socket closed: " << "reason :" << reason << "(" << errorCode << ")";
	Close();
}

// 웹소켓을 통해 수신되는 메시지 핸들러
int SignalClient::OnMessage(const string& strMsg)
{
	ELOGD << "################## receive msg : " << strMsg;
	stringstream ss(strMsg);

	Json::CharReaderBuilder reader;
	Json::Value jMsg;
	string errs;

	if (!Json::parseFromStream(reader, ss, &jMsg, &errs))
	{
		ELOGE << "parse_error: " << errs;
		return 0;
	}

	ELOGD << strMsg.size();
	Json::StreamWriterBuilder writer;
	ELOGD << Json::writeString(writer, jMsg);

	const Json::Value& jAction = jMsg["action"];
	if (jAction.empty())
	{
		ELOGE << "action is null";
		return 0;
	}

	string action = jAction.asCString();

	if (action == "join_stream")
	{
		const Json::Value& jKeep_alive_interval = jMsg["keep_alive_interval"];
		JCHK(jKeep_alive_interval, isUInt);

		m_nKeepAliveIntervalms = jKeep_alive_interval.asUInt() * 1000;

		StartKeepAliveTimer();

		// 내 정보를 얻는다
		const Json::Value& jScope = jMsg["room"]["scope"];
		JCHK(jScope, isString);
		m_stPeerInfo.scope = jScope.asString();

		const Json::Value& jRoomID = jMsg["room"]["room_id"];
		JCHK(jRoomID, isString);
		m_stPeerInfo.roomID = jRoomID.asString();

		const Json::Value& jRoomName = jMsg["room"]["room_name"];
		JCHK(jRoomName, isString);
		m_stPeerInfo.roomName = jRoomName.asString();

		const Json::Value& jMyID = jMsg["player"]["player_id"];
		JCHK(jMyID, isString);
		m_stPeerInfo.myID = jMyID.asString();

		const Json::Value& jMyName = jMsg["player"]["player_name"];
		JCHK(jMyName, isString);
		m_stPeerInfo.myName = jMyName.asString();

		if (m_pOwner)
		{
			m_pOwner->CallOnStarted();
		}
	}
	else if (action == "keep_alive")
	{

	}
	else if (action == "instruction_event")
	{
		return _OnMessage_InstructionEvent(jMsg);
	}
	else if (action == "info_event")
	{
		return _OnMessage_InfoEvent(jMsg);
	}

	ELOGD << "====== received action: " << action;

	return 0;
}

int	SignalClient::_OnMessage_InstructionEvent(const Json::Value& jMsg)
{
	const Json::Value& jEvent = jMsg["event"];
	JCHK(jEvent, isString);
	string event = jEvent.asString();

	ELOGD << "====== received action: " << "instruction_event :" << event;

	if (event == "do_sdp_exchange" || event == "do_confirm_and_sdp_exchange")
	{
		const Json::Value& jSdp_instruction = jMsg["sdp_instruction"];
		JCHK(jSdp_instruction, isString);
		string sdp_instruction = jSdp_instruction.asString();

		const Json::Value& jPeer_id = jMsg["peer_id"];
		JCHK(jPeer_id, isString);
		string peerID = jPeer_id.asString();

		if (sdp_instruction == "offer" || sdp_instruction == "answer")
		{
			vector<IceServerInfo> iceList;
			const Json::Value& jIcelist = jMsg["ice_servers"];
			JCHK(jIcelist, isArray);

			for (const Json::Value& jUrlGroup : jIcelist)
			{
				const Json::Value& jUrls = jUrlGroup["urls"];
				const Json::Value& jUsername = jUrlGroup["username"];
				const Json::Value& jCredential = jUrlGroup["credential"];
					
				JCHK(jUrls, isArray);
				for (const Json::Value& jUrl : jUrls)
				{
					JCHK(jUrl, isString);
					IceServerInfo ice_server;
					ice_server.url = jUrl.asString();
					ice_server.username = jUsername.asString();
					ice_server.password = jCredential.asString();
					iceList.push_back(ice_server);
				}
			}

			// offer/Answer에서만 PeerConnection 생성
			if (sdp_instruction == "offer")
			{
				PeerConnection* pPeerConnection = m_pOwner->FindPeerConnection(peerID, true, &iceList);
				pPeerConnection->StartOffer();
			}
			else
			{
				const Json::Value& jSdp = jMsg["jsep"]["sdp"];
				JCHK(jSdp, isString);
				string sdp = jSdp.asString();

				PeerConnection* pPeerConnection = m_pOwner->FindPeerConnection(peerID, true, &iceList);
				pPeerConnection->StartAnswer(sdp);
			}

		}
		else if (sdp_instruction == "accept")
		{
			const Json::Value& jSdp = jMsg["jsep"]["sdp"];
			JCHK(jSdp, isString);
			string sdp = jSdp.asString();

			PeerConnection* pPeerConnection = m_pOwner->FindPeerConnection(peerID, false);
			if (pPeerConnection == nullptr)
			{
				ELOGE << "Cannot found PeerConnection : " << peerID;
				return 0;
			}
			pPeerConnection->StartAccept(sdp);
		}
		else
		{
//			return INVALID_PARAM;
			ELOGE << "unknown sdp insturction : " << sdp_instruction;
			return 0;
		}
	}
	else if (event == "do_add_ice_candidate")
	{
		const Json::Value& jPeer_id = jMsg["peer_id"];
		JCHK(jPeer_id, isString);
		string peerID = jPeer_id.asString();

		const Json::Value& jCandidate = jMsg["ice_candidate"]["candidate"];
		JCHK(jCandidate, isString);
		string candidate = jCandidate.asString();

		const Json::Value& jSdpMid = jMsg["ice_candidate"]["sdpMid"];
		JCHK(jSdpMid, isString);
		string sdpMid = jSdpMid.asString();

		const Json::Value& jSdpMLine = jMsg["ice_candidate"]["sdpMLineIndex"];
		JCHK(jSdpMLine, isInt);
		int sdpMLine = jSdpMLine.asInt();

		PeerConnection* pPeerConnection = m_pOwner->FindPeerConnection(peerID, false);
		if (pPeerConnection == nullptr)
		{
			ELOGE << "Cannot found PeerConnection : " << peerID;
			return 0;
		}
		pPeerConnection->StartIceAdd(candidate, sdpMid, sdpMLine);
	}
	else if (event == "do_rejoin_stream")
	{
		/*
		미디어 서버 장애로인한 복구시도, 혹은 Player의 media_role 변경으로 인해 재접속이 요구될 경우 전파됩니다.
		모든 Peer Connection을 정리하고 새로 전달받은 auth_key 를 사용해 join_stream 부터 새로 접속을 수행하면 됩니다. (WebSocket은 다시 맺을 필요 없습니다.)
		*/
		const Json::Value& jAuthKey = jMsg["auth_key"];
		JCHK(jAuthKey, isString);
		string authKey = jAuthKey.asString();

		m_pOwner->RestartMediaRoom(authKey);
	}
	else if (event == "do_disconnect_peer")
	{
		/*
		P2P에서 kick으로 누군가 추방되는 등 명시적으로 Peer Connection을 끊어야 할 경우 전파됩니다.
		peer_id에 해당하는 Peer Connection을 정리하면 됩니다.
		peer_id 가 null일 경우에는 모든 Peer Connection을 정리합니다.
		service_type에 따라 미디어 서버측에서 이미 Peer Connection을 끊었을 수도 있습니다.
		*/
		const Json::Value& jPeer_id = jMsg["peer_id"];
		if (jPeer_id.empty())
		{
			m_pOwner->RemoveAllPeerConnection();
		}
		else
		{
			string peerID = jPeer_id.asString();
			m_pOwner->RemovePeerConnection(peerID);
		}
	}
	else if (event == "do_close")
	{
		/*
		Room이 destroy 되었을때 모든 참여자에게 전파됩니다.
		각종 Connection 리소스를 정리하시고 세션을 종료하면 됩니다.
		*/
		//m_pOwner->stopMediaRoom();
	}
	else
	{
		// TODO : Apply return Enum value
//				return INVALID_PARAM;
	}

	return 0;
}

int SignalClient::_OnMessage_InfoEvent(const Json::Value& jMsg)
{
	const Json::Value& jEvent = jMsg["event"];
	JCHK(jEvent, isString);
	string event = jEvent.asString();

	ELOGD << "====== received action: " << "info_event :" << event;

	if (event == "join_stream")
	{
/*
		const Json::Value& jEvent = jMsg["event"];
		JCHK(jEvent, isString);
		string event = jEvent.asString();

		// 유저 정보를 얻는다
		const Json::Value& jPlayerID = jMsg["player"]["player_id"];
		JCHK(jPlayerID, isString);
		string playerID = jPlayerID.asString();

		const Json::Value& jPlayerName = jMsg["player"]["player_name"];
		JCHK(jPlayerName, isString);
		string playerName = jPlayerName.asString();
*/
/*
		if (m_stPartiInfo.sMyId != conversions::utf8_to_utf16(jplayerid.asCString()))
		{
			// not me, but peer
			SParticipantInfo stPeerInfo;
			stPeerInfo = m_stPartiInfo;
			stPeerInfo.sMyId = conversions::utf8_to_utf16(jplayerid.asCString());
			stPeerInfo.sMyName = conversions::utf8_to_utf16(jplayername.asCString());

			g_Obsvr.Get().WMS_OnPeerJoin(m_stInvCard, stPeerInfo);
		}
*/
	}
	else if (event == "leave_room")
	{
		// 유저 정보를 얻는다
		const Json::Value& jPlayerID = jMsg["player"]["player_id"];
		JCHK(jPlayerID, isString);
		string playerID = jPlayerID.asString();

		const Json::Value& jPlayerName = jMsg["player"]["player_name"];
		JCHK(jPlayerName, isString);
		string playerName = jPlayerName.asString();

		m_pOwner->RemovePeerConnection(playerID);
/*
		if (m_stPartiInfo.sMyId != conversions::utf8_to_utf16(jplayerid.asCString()))
		{
			// not me, but peer
			SParticipantInfo stPeerInfo;
			stPeerInfo = m_stPartiInfo;
			stPeerInfo.sMyId = conversions::utf8_to_utf16(jplayerid.asCString());
			stPeerInfo.sMyName = conversions::utf8_to_utf16(jplayername.asCString());

			g_Obsvr.Get().WMS_OnPeerLeave(m_stInvCard, stPeerInfo);
		}
*/
	}
	else if (event == "destroy_room")
	{
	}
	else if (event == "change_media_status")
	{
	}
	else if (event == "kick_player")
	{
	}
	else if (event == "change_player_role")
	{
	}
	else if (event == "change_player_media_role")
	{
	}
	else if (event == "start_recording")
	{
	}
	else if (event == "stop_recording")
	{
	}

	return 0;
}

void CALLBACK SignalClient::KeepAliveProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	SignalClient* pSC = (SignalClient*)(dwUser);
	pSC->SendKeepAlive();
}

void SignalClient::StartKeepAliveTimer()
{
	StopKeepAliveTimer();

	SendKeepAlive();
	m_nTimerID = timeSetEvent((UINT)m_nKeepAliveIntervalms, 0, this->KeepAliveProc, (DWORD_PTR)this, TIME_PERIODIC);
}

void SignalClient::StopKeepAliveTimer()
{
	if (m_nTimerID != 0)
	{
		timeKillEvent(m_nTimerID);
	}
}
