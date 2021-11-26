#pragma once

#include <QtCore/qglobal.h>
#include "StompMessage.h"
#include <QtCore/QObject>
#include <QtWebSockets/QWebSocket>
#include <QtNetwork/QSslError>

#define NOMINMAX

#include <windows.h>
#include <mmsystem.h>
#pragma comment (lib, "Winmm.lib")


class StompReciever
{
public:
    virtual void OnConnectedWebSocket() = 0;
    virtual void OnConnected() = 0;	// COMMAND CONNECT
    virtual void OnDisconnected() = 0;
    virtual void OnMessage(const StompMessage &s) = 0;
	virtual void OnErrorMessage(const char* message) = 0;
    virtual void OnError(QAbstractSocket::SocketError error) = 0;
};

class QTWebStompClient : public QObject
{
public:
    /** Description : Constructor for the WebStompClient
    * Returns : Nothing
    * Parameters :
    - url: The url of the webstomp server (example: ws://localhost/ws). The ws:// to indicate the protocol is required. If you want to use ssl, then use wss:// .
           By default, the port is 80 but you can connect to any port using a url with a specific port.
    - login: Self-explanatory. The username you wanna use to log in
    - passcode: The password of the user. Why I didn't name it "password"? Well because in STOMP it's called passCODE.
    - vHost: The vHost you want to connect to. If you don't specify one, the server will try and connect you to the default one if you have access.
    - debug: Bool. Set to true to get qDebug messages to see what's going on.
    - parent: The parent of the QObject. I almost made you believe I know what this is!
    * @author : dmaurino
    */
    explicit QTWebStompClient(const char* url, const char* login, const char* passcode, StompReciever* receiver, const char* vHost = NULL, QObject *parent = Q_NULLPTR);
    ~QTWebStompClient();
    enum ConnectionState { Connecting, Connected, Subscribed, Closed };
    enum AckMode { Auto, Client, ClientIndividual};

	void ConnectSocket();
    void ConnectStomp();

    /** Description : Subscribes to a queue if you have permission to
    * Returns : Nothing
    * Parameters :
    - queueName: The name of the queue (example: /queue/MyQueue/)
    - onMessageCallback: The function that will execute when a message is received. It should have a signature of void (const StompMessage &stompMessage).
    - ackMode: How you want to ack the messages. If set to auto, the messages will be popped instantly from the queue. Other modes require you to ack messages manually.
    * @author : dmaurino
    */
    void Subscribe(const char* queueName, AckMode ackMode = Auto);

    /** Description : Acks a received stompMessage
    * Parameters:
    - s: The stompmessage instance you want to ack.
    **/
    void Ack(const StompMessage &s);

    /** Description : Acks a specific id
    * Parameters:
    - id: the id of the message you want to ack.
    **/
    void Ack(const char* id);

    /** Description : Sends an already constructed stompmessage.
    Use the other function overload (cause it's easier) unless you want full customization of the message **/
    void Send(const StompMessage & stompMessage);

    /** Description : Sends a message to a specific queue
    * Returns : Nothing.
    * Parameters :
    - destination: A c string with the name of the queue (example: /queue/MyQueue/)
    - message: A c string containing the message (can be anything, like a json)
    - headers: (Optional) if you want to include a specific header, you can do it using this. Pass a map reference.
    * @author : dmaurino
    */
    void Send(const char* destination, const char* message, map<std::string, std::string> &headers/* = map<std::string, std::string>()*/);
    void Send(const char* destination, string tid, string message, string language);

    ConnectionState GetConnectionState() { return m_connectionState; }

Q_SIGNALS:
    void closed();

    private Q_SLOTS:
    void onConnected();
    void onTextMessageReceived(QString message);
    void onSslErrors(const QList<QSslError> &errors);
    void onError(QAbstractSocket::SocketError error);
public: QWebSocket* getWebSocket() {return &m_webSocket;}


private:
    QWebSocket m_webSocket;
    QUrl m_url;
    bool m_SSL;
    ConnectionState m_connectionState;
    const char* m_login;
    const char* m_passcode;
    const char* m_vHost;
    std::string m_token;
    StompReciever*	m_pReceiver;

    // timer
    short			m_nKeepAliveIntervalms;
    unsigned int		m_nTimerID;

    void			SendKeepAlive();
    static void CALLBACK	KeepAliveProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

    void			StartKeepAliveTimer();
    void			StopKeepAliveTimer();
};
