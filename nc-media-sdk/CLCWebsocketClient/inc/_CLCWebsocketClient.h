#pragma once

/*
	CPPRESTSDK의 Websocket cliet 랩핑 클래스
*/

#include "common.h"
#include "CLCWebsocketClientError.h"

#include <string>

using namespace std;
enum SocketState
{
	SS_None,
//	SS_Connecting,
	SS_Connected,
	SS_Disconnected,
//	SS_Reconnecting,
};


class CLCWEBSOCKETCLIENT_API CLCWebsocketClient
{
public:
	CLCWebsocketClient();
	virtual ~CLCWebsocketClient();

	WSC_Result			Connect(const string& url, const string& authKey);

	WSC_Result			_SendMessage(const string& msg);

protected:
	virtual void		OnClose(const string& reason, int errorCode);

private:
	virtual int			OnMessage(const string& msg);
	void				Destroy();
	
	void*				m_websocketClient = nullptr;
	SocketState			m_state = SocketState::SS_None;
};
