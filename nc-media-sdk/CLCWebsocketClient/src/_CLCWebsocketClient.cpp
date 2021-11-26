#include "stdafx.h"

#include "..\inc\_CLCWebsocketClient.h"

//#define _NO_ASYNRTIMP
#include "cpprest/ws_client.h"
using namespace web;
using namespace web::websockets::client;
using namespace utility;

CLCWebsocketClient::CLCWebsocketClient()
{
	m_websocketClient = nullptr;
}


CLCWebsocketClient::~CLCWebsocketClient()
{
	Destroy();
}

void CLCWebsocketClient::Destroy()
{
	static_cast<websocket_callback_client*>(m_websocketClient)->close().wait();
	if (m_websocketClient)
	{
		delete static_cast<websocket_callback_client*>(m_websocketClient);
		m_websocketClient = nullptr;
	}
	m_state = SocketState::SS_None;
}

WSC_Result CLCWebsocketClient::Connect(const string& url, const string& authKey)
{
	// 이미 생성되어 있으면 리턴
	if (m_websocketClient)
		return WSC_Result::WSC_OK;

	if (SocketState::SS_Connected == m_state)
		return WSC_Result::WSC_OK;
	// 
	// websocket setup =================================================
	websocket_client_config cfg;
	cfg.set_validate_certificates(false);

	m_websocketClient = new websocket_callback_client(cfg);
	WSC_Result re = WSC_Result::WSC_OK;
	
	// set message handler ========================================
	static_cast<websocket_callback_client*>(m_websocketClient)->set_message_handler([this, &re](const websocket_incoming_message& msg)
	{
		try
		{
			msg.extract_string().then([this](string body) {	this->OnMessage(body.c_str()); }).wait();
		}
		catch (const exception& e) 
		{
			string s = e.what();
			re = WSC_Result::WSC_SetMessageHandler_Fail;
		}
	});

	if (re != WSC_Result::WSC_OK)
		return re;

	// connect to signal server ========================================
	m_state = SocketState::SS_None;
	try {
		static_cast<websocket_callback_client*>(m_websocketClient)->connect(conversions::to_utf16string(url)).then([this, &url]() {
			m_state = SocketState::SS_Connected;
		}).wait();
	}
	catch (const exception &e) {
		string s = e.what();
		return WSC_Result::WSC_Connect_Fail;
	}

	// set close handler ========================================
	const utility::string_t close_reason = U("Too large");
	static_cast<websocket_callback_client*>(m_websocketClient)->set_close_handler([this, url, authKey, &close_reason](websocket_close_status status,
		const utility::string_t& reason,
		const std::error_code& code) {
		std::string _reason(conversions::to_utf8string(reason));
		this->OnClose(_reason, code.value());
		m_state = SocketState::SS_Disconnected;
	});

	return WSC_Result::WSC_OK;
}

WSC_Result CLCWebsocketClient::_SendMessage(const string& msg)
{
	if (m_websocketClient == nullptr)
		return WSC_Result::WSC_OK;
	if (m_state != SocketState::SS_Connected)
		return WSC_Result::WSC_OK;

	websocket_outgoing_message sendMsg;
	sendMsg.set_utf8_message(msg);

	try
	{
		static_cast<websocket_callback_client*>(m_websocketClient)->send(sendMsg);
	}
	catch (const exception& e)
	{
		string s = e.what();
		return WSC_Result::WSC_SendMsg_Fail;
	}

	return WSC_Result::WSC_OK;
}

void CLCWebsocketClient::OnClose(const string & reason, int errorCode)
{
//	ELOGD << "socket closed: " << "reason :" << reason << "(" << errorCode << ")";
}

int CLCWebsocketClient::OnMessage(const string& msg)
{
	return 0;
}
