//
//  CommunityLiveError.h
//  CommunityLiveCore
//
//  Created by Taiky on 2019. 5. 10..
//  Copyright © NCSOFT
//
#pragma once

#include <string>

using namespace std;

enum eMsgType
{
	MT_Unknown,
	MT_Notify,
	MT_Error,
};

// define msg domain =========================================
//#define DOMAIN_PEER_CONNECTION_CLIENT	"PeerConnectionClient"
#define DOMAIN_MEDIAROOM				"MediaRoom"
#define DOMAIN_SIGNAL_CLIENT			"SignalClient"

// define msg uri =========================================
//#define DOMAIN_PEER_CONNECTION_CLIENT	"PeerConnectionClient"
#define URI_START_MEDIA_ROOM				"START_MEDIA_ROOM"
#define URI_SIGNAL_CLIENT_CONNECTION		"SIGNAL_CLIENT_CONNECTION"
#define URI_SIGNAL_RECOVERY					"RECOVERY"
#define URI_PEER_CONNECTION					"PEER_CONNECTION"



enum eErrorCode
{
	ERROR_OK					= 0,
    EC_Unknown					= 600000,
	NotInitialized				= 600001,
	InvalidParameter			= 600002,
    
    InvalidResponse				= 600010,
    ConnectionFailed			= 602010,
    NotConnected				= 602011,
    InvalidData					= 602012,
    Disconnected				= 602013,
    
    PC_CreateFailed				= 602014,
    PC_Disconnected             = 602015,
    PC_CreateSDPFailed          = 602016,
    PC_SetSDPFailed             = 602017,
    PC_ConnectionFailed         = 602018,
    PC_RecoveryTimeout          = 602019,
	PC_WeirdMediaStream			= 602020,
	PC_ConnectionClosed			= 602021,
	PC_ConnectionDisconnected	= 602022,

	WEBRTC_SocketServerCreateFailed				= 630001,
	WEBRTC_SocketServerThreadCreateFailed		= 630002,
	WEBRTC_PeerConnectionFactoryCreateFailed	= 630003,

};
