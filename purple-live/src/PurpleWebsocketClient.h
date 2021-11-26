#pragma once

#include <QtCore/QObject>
#include <QtWebSockets/QWebSocket>
#include <QtNetwork/QSslError>
#include <string>

using namespace std;

//
//	PurpleWebsocketClient 
//	Role : Purple agent communication
//
class PurpleWebsocketClient : public QObject
{
	Q_OBJECT
public:
	explicit PurpleWebsocketClient(const int port, const char* instanceName, bool debug = false, QObject* parent = nullptr);
	~PurpleWebsocketClient();

//Q_SIGNALS:
//	void	closed();

	void	SendStreamStatus(bool start);
	void	SendScreenUnlock();

private Q_SLOTS:
	void	onConnected();
	void	onDisconnected();
	void	onError(QAbstractSocket::SocketError error);
	void	onTextMessageReceived(QString message);

private:
	void	_SendClientInfo();

	QWebSocket	m_webSocket;
	QUrl		m_url;
	bool		m_debug;
	string		m_instanceName;
	int			m_port;
};