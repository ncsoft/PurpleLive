#include "PurpleWebsocketClient.h"
#include "easylogging++.h"
#include <json.h>
#include "VideoChatApp.h"
#include "MainWindow.h"
#include "AppString.h"
#include "AppUtils.h"

PurpleWebsocketClient::PurpleWebsocketClient(const int port, const char* instanceName, bool debug, QObject* parent) :
	QObject(parent),
	m_port(port),
	m_instanceName(instanceName),
	m_debug(debug)
{

	connect(&m_webSocket, &QWebSocket::connected, this, &PurpleWebsocketClient::onConnected);
	connect(&m_webSocket, &QWebSocket::disconnected, this, &PurpleWebsocketClient::onDisconnected);
	connect(&m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, &PurpleWebsocketClient::onError);

	string url = "ws://[::1]:" + to_string(m_port);
	m_url = QUrl(url.c_str());

	LOG(DEBUG) << "[PurpleWebsocketClient] : WebSocket Server : " << url.c_str();

	m_webSocket.open(m_url);
}

PurpleWebsocketClient::~PurpleWebsocketClient()
{
	m_webSocket.close();
}

void PurpleWebsocketClient::SendStreamStatus(bool start)
{
	Json::Value root;
	Json::Value j_msg;

	Json::StyledWriter writer;

	root["cmd"] = "purpleLiveMessage";
	root["gameHwnd"] = App()->GetGameHwndString().c_str();

	j_msg["cmd"]	= "broadcast";
	int code = start == true ? 0 : 1;
	j_msg["code"] = code;
	string _msg = writer.write(j_msg);

	root["message"] = _msg;
	
	string str = writer.write(root);

	LOG(DEBUG) << "[PurpleWebsocketClient] : Send StreamStatus: " << str;

	QString msg = QString::fromStdString(str);
	m_webSocket.sendTextMessage(msg);
}

void PurpleWebsocketClient::SendScreenUnlock()
{
	Json::Value root;

	root["cmd"] = "runPurple";
	eServiceNetwork esn = App()->GetServiceNetwork();
	string sn = "live";
	if (esn == eServiceNetwork::eSN_LIVE)
	{
		sn = "live";
	}
	else if (esn == eServiceNetwork::eSN_RC)
	{
		sn = "rc";
	}
	else if (esn == eServiceNetwork::eSN_SB)
	{
		sn = "sandbox";
	}
	else if (esn == eServiceNetwork::eSN_OP)
	{
		sn = "op";
	}
	root["serviceNetwork"] = sn.c_str();

	Json::StyledWriter writer;
	string str = writer.write(root);

	LOG(DEBUG) << "[PurpleWebsocketClient] : Send ScreenUnlock: " << str;

	QString msg = QString::fromStdString(str);
	m_webSocket.sendTextMessage(msg);
}

void PurpleWebsocketClient::onDisconnected()
{
	LOG(DEBUG) << "[PurpleWebsocketClient] : WebSocket disconnected";

	QWebSocketProtocol::CloseCode code = m_webSocket.closeCode();

	if (code != QWebSocketProtocol::CloseCode::CloseCodeNormal)
	{
		//retry connect
		LOG(DEBUG) << "[PurpleWebsocketClient] : retry connect";
		m_webSocket.open(m_url);
	}
}

void PurpleWebsocketClient::onConnected()
{
	LOG(DEBUG) << "[PurpleWebsocketClient] : WebSocket connected";
	connect(&m_webSocket, &QWebSocket::textMessageReceived,	this, &PurpleWebsocketClient::onTextMessageReceived);

	_SendClientInfo();
}

void PurpleWebsocketClient::onError(QAbstractSocket::SocketError error)
{
	LOG(ERROR) << "[PurpleWebsocketClient] : WebSocket error : " << m_webSocket.errorString().toStdString().c_str();
	connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &PurpleWebsocketClient::onTextMessageReceived);
}

void PurpleWebsocketClient::onTextMessageReceived(QString message)
{
	Json::Reader reader;
	Json::Value root;

	if (reader.parse(message.toStdString().c_str(), root) == false)
		LOG(ERROR) << "[PurpleWebsocketClient::onTextMessageReceived] agent's message read failed";

	const Json::Value& jCmd = root["cmd"];
	if (jCmd.empty())
	{
		LOG(ERROR) << "[PurpleWebsocketClient::onTextMessageReceived] unknown message";
		return;
	}

	string cmd = jCmd.asString();

	LOG(DEBUG) << "[PurpleWebsocketClient]::onTextMessageReceived : " << message.toStdString().c_str();

	if (cmd == "bosskey")
	{
		string success		= root["success"].asString();
		string needShow		= root["needShow"].asString();

		if (needShow == "true")
		{
			MainWindow::Get()->SetBossHide(false);
		}
		else
		{
			MainWindow::Get()->SetBossHide(true);
		}
	}
	else if (cmd == "screenLock")
	{
		string account		= root["account"].asString();
		string instanceName = root["instanceName"].asString();
		string lock			= root["lock"].asString();
		string success		= root["success"].asString();

		if (lock == "True")
		{
			MainWindow::Get()->SetScreenLock(true);
		}
		else
		{
			MainWindow::Get()->SetScreenLock(false);
		}
	}
	else if (cmd == "closeNotice")
	{
		string account		= root["account"].asString();
		string gameId		= root["gameId"].asString();
		string instanceName = root["instanceName"].asString();
		string success		= root["success"].asString();

		if (App()->GetInstanceName() == instanceName && App()->GetGameId() == gameId)
		{
			LOG(DEBUG) << "[PurpleWebsocketClient] : closeNotice : " << gameId.c_str() << " : " << instanceName.c_str();
			AppUtils::DispatchToMainThread([this] {
				MainWindow::Get()->AppClose(AppString::GetString("event_message/event_game_closed").c_str());
			});
		}
	}
	else if (cmd == "common")
	{
		string account	= root["account"].asString();
		string msg		= root["message"].asString();

		Json::Value j_message;

		if (reader.parse(msg.c_str(), j_message) == false)
		{
			LOG(ERROR) << "[PurpleWebsocketClient::onTextMessageReceived] agent's message read failed";
			return;
		}

		string command	= j_message["cmd"].asString();
		string args		= j_message["arg"].asString();
		string callback	= j_message["callback"].asString();

		if (command == "setPurpleLanguage")
		{
			if (App()->GetLocalLanguage()!=QString(args.c_str()))
			{
				// change language
				App()->SetLocalLanguage(args.c_str());
			}
		}
	}
	else
	{

	}
}

void PurpleWebsocketClient::_SendClientInfo()
{
	Json::Value root;

	root["cmd"] = "setClientInfo";
	root["gameId"] = "PURPLELIVE";
	root["instanceName"] = App()->GetInstanceName().c_str();
	root["appGroupcode"] = App()->GetGameCode().c_str();

	Json::StyledWriter writer;
	string str = writer.write(root);

	LOG(DEBUG) << "[PurpleWebsocketClient] : Send ClientInfo: " << str;

	QString msg = QString::fromStdString(str);
	m_webSocket.sendTextMessage(msg);
}
