#pragma once

#include <string>
#include <QNetworkAccessManager>
#define NOMINMAX
#include <windows.h>
#undef NOMINMAX

#include "lime-defines.h"

using namespace std;

// API GATE URL
#define API_GATE_RC "https://***********"
#define API_GATE_SANDBOX "https://***********"
#define API_GATE_OP "https://***********"
#define API_GATE_LIVE "https://***********"

#define LOGIN_RC "https://***********"

namespace {
	std::string rsa_priv_key = R"(-----BEGIN RSA PRIVATE KEY-----
***********
-----END RSA PRIVATE KEY-----)";
}

#define MAP_LOG_ID_INSTALL	1001
#define MAP_LOG_ID_START	1003
#define MAP_LOG_ID_END		1004
#define MAP_LOG_ID_CUSTOM	1006

class MAPManager : public QObject
{
	Q_OBJECT
public:
	virtual ~MAPManager();

	static MAPManager* Instance()
	{
		static MAPManager instance;
		return &instance;
	}

	void	LogIn(const string& authn_token, const string& appGroupCode, const string& uuid, const string& version); // 런처에서 넘겨받은 authn_token으로 apiGate에 로그인하여 session키를 받는다

	bool	LogInstall();		//최초 설치시
	bool	LogStart();			//시작시
	bool	LogEnd();			//종료시
	bool	LogStartStream(bool useCam, bool useMic);	//스트리밍 시작시
	bool	LogStopStream(bool useCam, bool useMic);	//스트리밍 종료시
	bool	LogStatusMic(bool active, const string& deviceName);
	bool	LogStatusCam(bool active, const string& deviceName);
	bool	LogAddCaption(const string& caption);

	string	MakeUUID();

	QNetworkAccessManager*	GetNetworkAcessManager() { return m_pManager; }

	string	GetAdid() { return m_adid; }

public slots:
	bool	LogKeepAlive();		//keep alive

private:
	explicit MAPManager();

	void	Destroy();
	bool	PostLog(const string& msg);
	bool	PostLogSync(const string& msg);
	bool	waitForConnect(int nTimeOutms, QNetworkReply* pReply);
	void	slotWaitTimeout();

	void	OnError(QNetworkReply* reply);

	const string GetApiGateURL(eServiceNetwork sn);

	// timer
	static void CALLBACK KeepAliveProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
	void		StartKeepAliveTimer();
	void		StopKeepAliveTimer();
	unsigned int	m_nKeepAliveIntervalms;
	UINT		m_nTimerID;

	void		GetSystemInfo();
	string		GetDateTime();
	long		GetTransactionID();

	QNetworkAccessManager* m_pManager;
	string		m_session_id;
	string		m_user_id;
	string		m_uuid;
	string		m_appID;
	string		m_adid;
	string		m_cpuInfo;
	string		m_osInfo;
	string		m_locale;
	string		m_version;
	string		m_appGroupCode;

	bool		m_login = false;
	bool		m_EndLogTimeout = false;

	long		m_tid = 0;
};

