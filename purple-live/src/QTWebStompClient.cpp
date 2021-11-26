#pragma once
#include "QTWebStompClient.h"
#include "easylogging++.h"
#include "AppUtils.h"

using namespace std;

StompMessage::StompMessage(const char* rawMessage)
{
    std::string strMessage(rawMessage);
    vector<std::string> messageVector = messageToVector(strMessage, "\n");
    m_message = messageVector.back(); // deep copies
    messageVector.pop_back();
    bool first = true;
    for (const auto& header : messageVector)
    {
        size_t pos = header.find_first_of(':', 0);
        std::string key = header.substr(0, pos);
        if (first)
        {
            m_messageType = key;
            first = false;
            continue;
        }
        std::string value = header.substr(++pos, value.length() - pos);
        m_headers[key] = value;
    }
}

StompMessage::StompMessage(string messageType, map<string, string> headers, const char* messageBody)
{
    m_messageType = std::string(messageType);
    m_message = messageBody;
    m_headers = headers;
}

std::string StompMessage::toString() const
{
    std::string result = m_messageType + "\u000A";
    for (const auto &header : m_headers)
    {
        result += header.first + ":" + header.second + "\u000A";
    }

    result += "\u000A" + m_message + "\u0000";

    return result;
}

vector<string> StompMessage::messageToVector(const string& str, const string& delim)
{
    vector<string> messageParts;
    size_t prev = 0, pos = 0;
    bool last = false;
    do
    {
        if (last)
        {
            auto message = str.substr(prev, str.length() - prev);
            messageParts.push_back(message);
            break;
        }

        pos = str.find(delim, prev);
        if (pos == string::npos) pos = str.length();
        string token = str.substr(prev, pos - prev);
        if (!token.empty())
        {
            messageParts.push_back(token);
        }
        else
        {
            // If the token is empty the headers finished =) Just the body left!
            last = true;
        }
        prev = pos + delim.length();
    } while (pos < str.length() && prev <= str.length()); // I know but we have to do it at least once, so...
    return messageParts;
}


////////////////////////////////////////////

QTWebStompClient::QTWebStompClient(const char* url, const char* login, const char* passcode, StompReciever* receiver, const char* vHost, QObject *parent)
{
    UNREFERENCED_PARAMETER(parent);

    m_login = login;
    m_passcode = passcode;
    m_pReceiver = receiver;
    m_nTimerID = 0;

    LOG(DEBUG) << "Connecting to WebSocket server:" << url;
    
    m_vHost = "";
    m_token = vHost;
	m_url = QUrl(url);

    connect(&m_webSocket, &QWebSocket::connected, this, &QTWebStompClient::onConnected);
    connect(&m_webSocket, (&QWebSocket::sslErrors),	this, &QTWebStompClient::onSslErrors);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &QTWebStompClient::closed);
}

QTWebStompClient::~QTWebStompClient()
{
	m_webSocket.close();
}

void QTWebStompClient::onConnected()
{
    LOG(DEBUG) << "-----------------------" << endl << "Connected to Websocket!" << endl << "-----------------------" << endl;
    
	connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &QTWebStompClient::onTextMessageReceived);

    m_connectionState = Connecting;

    m_pReceiver->OnConnectedWebSocket();

    // 10초마다 heart beat 보냄
    m_nKeepAliveIntervalms = 10000;
    StartKeepAliveTimer();
}

void QTWebStompClient::ConnectSocket()
{
	connect(&m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, &QTWebStompClient::onError);
	m_webSocket.open(m_url);
}

void QTWebStompClient::ConnectStomp()
{
    map<string, string> headers;
    headers["login"] = m_login;
    headers["passcode"] = m_passcode;
    headers["Authorization"] = m_token.c_str();
    headers["accept-version"] = "1.0,1.1,1.2";
    headers["heart-beat"] = "10000,10000";

    StompMessage message("CONNECT", headers, "");

    auto connectMsg = QString(message.toString().c_str());
    QString frame = QString(connectMsg.data(), connectMsg.size() + 1);

    m_webSocket.sendTextMessage(frame);

	LOG(DEBUG) << "------------Send1(STOMP)------------" << endl << StringUtils::utf8_to_string(frame.toStdString()) << endl;
}

void QTWebStompClient::onTextMessageReceived(QString message)
{
    if (message.toStdString() == "\n")
        return;

    LOG(DEBUG) << "------------Recv(STOMP)------------" << endl << StringUtils::utf8_to_string(message.toStdString()) << endl;
	
    StompMessage stompMessage(message.toStdString().c_str());

    switch (m_connectionState)
    {
        case Connecting:
            //LOG(DEBUG) << "Connection response: " << StringUtils::utf8_to_string(stompMessage.toString());
            
            if (stompMessage.m_messageType == "CONNECTED")
            {
                LOG(DEBUG) << "--------------------" << endl << "Connected to STOMP!" << endl << "--------------------" << endl;
                
				m_connectionState = Connected;
                m_pReceiver->OnConnected();
            }
            else
            {
				std::string error{ "Message type CONNECTED expected, got" + StringUtils::utf8_to_string(stompMessage.toString()) };
                m_pReceiver->OnErrorMessage(error.c_str());
            }
            break;

        case Subscribed:
            // TODO: Improve check, maybe different messages are allowed when subscribed
            if (stompMessage.m_messageType == "MESSAGE")
            {
                //LOG(DEBUG) << "Message received from queue!" << endl << StringUtils::utf8_to_string(stompMessage.toString());
                
                if (m_pReceiver)
                    m_pReceiver->OnMessage(stompMessage);
            }
            else
            {
				std::string error{ "Message type CONNECTED expected, got" + StringUtils::utf8_to_string(stompMessage.toString()) };
				m_pReceiver->OnErrorMessage(error.c_str());
            }
            break;

        default:
			std::string error{ "Unsupported connection state" };
			m_pReceiver->OnErrorMessage(error.c_str());
            break;
    }
}

// TODO: Change to have multiple ids (so we can handle more than one subscription)
void QTWebStompClient::Subscribe(const char* queueName, QTWebStompClient::AckMode ackMode)
{
    UNREFERENCED_PARAMETER(ackMode);

    if (m_connectionState != Connected)
    {
        // For now, if you need to subscribe to 2 queues, you can create two instances of the client. Later an improvement would be to use the id variables of the underlying websocket lib.
		std::string error{ "Cannot subscribe when connection hasn't finished or when already subscribed. Try using the callback function for onConnect to subscribe" };
		m_pReceiver->OnErrorMessage(error.c_str());
    }
    map<string, string> headers;
    headers["id"] = "sub-0";
    headers["destination"] = std::string(queueName);
/*
    switch (ackMode)
    {
        case Client:
            headers["ack"] = "client";
            break;

        case ClientIndividual:
            headers["ack"] = "client-individual";
            break;

        default:
            headers["ack"] = "auto";
            break;
    }
*/
    StompMessage myMessage("SUBSCRIBE", headers, "");

    auto subscribeMessage = QString(myMessage.toString().c_str());
    QString frame = QString(subscribeMessage.data(), subscribeMessage.size() + 1);

    m_webSocket.sendTextMessage(frame);
    m_connectionState = Subscribed;

    LOG(DEBUG) << "------------Send2(STOMP)------------" << endl << StringUtils::utf8_to_string(frame.toStdString()) << endl;
}

void QTWebStompClient::onSslErrors(const QList<QSslError> &errors)
{
    UNREFERENCED_PARAMETER(errors);
    std::string error{ "SSL error! I'd show the error description if there were an easy way to convert from enum to string in c++. You'll have to debug." };
	m_pReceiver->OnErrorMessage(error.c_str());
}

void QTWebStompClient::onError(QAbstractSocket::SocketError error)
{
    m_pReceiver->OnError(error);

    LOG(DEBUG) << format_string("Stomp Websocket Error (%d)", error);
}

void QTWebStompClient::closed()
{
    // 객체가 삭제될 때와 disconnect때 모두 호출됨
    StopKeepAliveTimer();

	QWebSocketProtocol::CloseCode code = m_webSocket.closeCode();

	// 정상 종료일때는 OnDisconnected()를 호출하지 않음
	if (code == QWebSocketProtocol::CloseCode::CloseCodeNormal)
		return;

    m_pReceiver->OnDisconnected();
    LOG(DEBUG) << "Stomp Websocket Connection closed";
}

void QTWebStompClient::Ack(const StompMessage & s)
{
    auto ack = s.m_headers.at(std::string("ack"));
    Ack(ack.c_str());
}

void QTWebStompClient::Ack(const char* id)
{
    //LOG(DEBUG) << "Acking message with id: " << id;
    // Yes I know that this is disgusting
    QString ackFrame("ACK\u000Aid:{{TheAckId}}\u000A\u000A\u000A\u0000");
    ackFrame.replace("{{TheAckId}}", id);
    QString frame(ackFrame.data(), ackFrame.size() + 1); // solves the null-terminator issue
    m_webSocket.sendTextMessage(frame);

    LOG(DEBUG) << "------------Send3(STOMP)------------" << endl << StringUtils::utf8_to_string(frame.toStdString()) << endl;
}

void QTWebStompClient::Send(const StompMessage & stompMessage)
{
    //LOG(DEBUG) << "Sending message: " << stompMessage.toString().c_str();
    
    std::string sendFrame = std::string(stompMessage.m_messageType+"\u000A");
    for (auto &header : stompMessage.m_headers)
    {
        sendFrame += header.first + ":" + header.second + "\u000A";
    }

    sendFrame += "\u000A" + stompMessage.m_message + "\u0000";

    QString sendFrameTemp(sendFrame.c_str());
    QString frame(sendFrameTemp.data(), sendFrameTemp.size() + 1);
    m_webSocket.sendTextMessage(frame);

    LOG(DEBUG) << "------------Send4(STOMP)------------" << endl << StringUtils::utf8_to_string(frame.toStdString()) << endl;
}

void QTWebStompClient::Send(const char* destination, const char* message, map<std::string, std::string> &headers)
{
    headers[std::string("destination")] = std::string(destination);

    StompMessage s("SEND", headers, message);
    Send(s);
}

void QTWebStompClient::Send(const char* destination, string tid, string message, string language)
{
    map<string, string> headers;

    headers["destination"] = destination;
    headers["content-type"] = "application/json;charset=utf-8";
    headers["Lime-Trace-Id"] = tid;
    headers["Lime-API-Version"] = "100";
    headers["content-length"] = std::to_string(message.length());
	headers["Lime-Locale"] = language;
    StompMessage msg("SEND", headers, message.c_str());

	//LOG(DEBUG) << format_string("[QTWebStompClient::Send] message(%s)", message.c_str());
	
    Send(msg);
}

void QTWebStompClient::SendKeepAlive()
{
    QString frame("\n");
    m_webSocket.sendTextMessage(frame);
}

void CALLBACK QTWebStompClient::KeepAliveProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
    UNREFERENCED_PARAMETER(uTimerID);
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(dw1);
    UNREFERENCED_PARAMETER(dw2);
    QTWebStompClient* pSC = (QTWebStompClient*)(dwUser);
    pSC->SendKeepAlive();
}

void QTWebStompClient::StartKeepAliveTimer()
{
    StopKeepAliveTimer();

    m_nTimerID = timeSetEvent((UINT)m_nKeepAliveIntervalms, 0, this->KeepAliveProc, (DWORD_PTR)this, TIME_PERIODIC);
}

void QTWebStompClient::StopKeepAliveTimer()
{
    if (m_nTimerID != 0)
    {
        timeKillEvent(m_nTimerID);
    }
}