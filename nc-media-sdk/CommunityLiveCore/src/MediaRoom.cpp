#include "MediaRoom.h"
#include <vector>
#include "SignalClient.h"
#include "PeerConnection.h"
#include "WebRTCManager.h"
#include "WebRTCHelper.h"
#include "CommunityLiveError.h"
#include "easylogging++.h"

void MediaRoom::CallOnStarted()
{
	if (m_pMediaRoomDelegate)
		m_pMediaRoomDelegate(m_pDelegateOwner, eMsgType::MT_Notify, DOMAIN_MEDIAROOM, "StartMediaRoom", 0, "Started");
}

void MediaRoom::CallOnStopped()
{
	ELOGD << "=======================================";
	ELOGD << "MediaRoom Stop";
	ELOGD << "=======================================";
	if (m_pMediaRoomDelegate)
		m_pMediaRoomDelegate(m_pDelegateOwner, eMsgType::MT_Notify, DOMAIN_MEDIAROOM, "StopMediaRoom", 0, "Stopped");

}

void MediaRoom::CallOnNotify(string domain, string uri, string msg)
{
	if (m_pMediaRoomDelegate)
		m_pMediaRoomDelegate(m_pDelegateOwner, eMsgType::MT_Notify, domain.c_str(), uri.c_str(), 0, msg.c_str());
}

void MediaRoom::CallOnError(string domain, string uri, int32_t error, string msg)
{
	if (m_pMediaRoomDelegate)
		m_pMediaRoomDelegate(m_pDelegateOwner, eMsgType::MT_Error, domain.c_str(), uri.c_str(), error, msg.c_str());
}

