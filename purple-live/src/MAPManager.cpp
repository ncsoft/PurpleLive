#include "MAPManager.h"
#include "easylogging++.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QTextCodec>
#include <QTimer>
#include <QEventLoop>
#include "jwt-cpp/jwt.h"
#include <sys/timeb.h>
#include "SystemInfo.h"
#include <IPHlpApi.h>                       // for GetAdaptersInfo()
#include "AppConfig.h"
#include "VideoChatApp.h"

#pragma comment(lib, "iphlpapi.lib" )

MAPManager::MAPManager()
{
	m_login = false;
	m_pManager = new QNetworkAccessManager(nullptr);
	m_tid = 0;
	m_nKeepAliveIntervalms = 60*40*1000;
	m_nTimerID = 0;
	GetSystemInfo();
}

MAPManager::~MAPManager()
{
	Destroy();
}

void debugRequest(QNetworkRequest request, QByteArray data = QByteArray())
{
	LOG(DEBUG) << request.url().toString().toStdString();

	QList<QByteArray> reqHeaders = request.rawHeaderList();
	foreach(QByteArray reqName, reqHeaders)
	{
		QByteArray reqValue = request.rawHeader(reqName);
		LOG(DEBUG) << reqName.toStdString() << ": " << reqValue.toStdString();
	}
	LOG(DEBUG)<< data.toStdString();
}

void MAPManager::LogIn(const string& authn_token, const string& appGroupCode, const string& uuid, const string& version)
{
	m_uuid = uuid;
	m_appID = AppConfig::APP_ID;
	m_version = version;
	m_appGroupCode = appGroupCode;

	// make body =====================================================
	QJsonObject root;

	root["auth_provider_code"] = "np";
	root["authn_token"] = authn_token.c_str();
	root["persistent"] = 0;

	QJsonDocument doc(root);
	string req = doc.toJson().toStdString();

	// generate JWT =====================================================
	auto token = jwt::create()
		.set_type("JWT")
		.set_algorithm("RS256")
		.set_payload_claim("iss", jwt::claim(m_appID))
		.set_payload_claim("iat", jwt::claim(std::chrono::system_clock::now()))
		.set_payload_claim("typ", jwt::claim(std::string("session")))
		.sign(jwt::algorithm::rs256{ "", rsa_priv_key, "", "***********" });

	string strURL = format_string("%s/sessions/v1.0/sessions;token", GetApiGateURL(App()->GetServiceNetwork()));
	QUrl url = QString::fromUtf8(strURL.c_str());
	QNetworkRequest request(url);

	request.setRawHeader("Accept", "application/json");
	request.setRawHeader("Accept-Encoding", "identity");
	request.setRawHeader("Content-Length", format_string("%d", req.length()).c_str());
	request.setRawHeader("Content-Type", "application/json; charset=utf-8");
	request.setRawHeader("Authorization", format_string("JWT %s", token.c_str()).c_str());

	debugRequest(request, req.c_str());

	QNetworkReply* reply = m_pManager->post(request, req.c_str());
	QObject::connect(reply, &QNetworkReply::finished, [=]()
	{
		if (reply->error() == QNetworkReply::NoError)
		{
			QByteArray response = reply->readAll();
			LOG(DEBUG) << "OK:" << response.toStdString();

			QJsonDocument body(QJsonDocument::fromJson(response));
			QJsonObject root = body.object();
			m_session_id = root["session"].toString().toStdString();
			m_user_id = root["user_id"].toString().toStdString();

			m_login = true;

			LogInstall();
			LogStart();
		}
		else // handle error
		{
			m_login = false;
			OnError(reply);
		}
	});
}

void MAPManager::Destroy()
{
	if (m_pManager)
	{
		delete m_pManager;
		m_pManager = nullptr;
	}
}

bool MAPManager::LogInstall()
{
	if (m_login == false)
		return false;

	QJsonObject root;
	QJsonObject log;
	QJsonObject command;
	QJsonObject info;
	QJsonObject user_s;
	QJsonArray logArray;

	// setup command 
	command["adid"] = m_adid.c_str();
	log.insert("install", command);

	// setup info
	info["os"] = "NGP_WIN";
	info["osver"] = QString::fromLocal8Bit(m_osInfo.c_str());
	info["did"] = m_uuid.c_str();
	info["md"] = m_cpuInfo.c_str();
	info["ctver"] = m_version.c_str();
	info["country"] = m_locale.c_str();
	info["store"] = "ncsoft";
	info["uid"] = m_user_id.c_str();
	info["oauth"] = "np";
	info["appid"] = m_appID.c_str();
	info["ug1"] = "";
	log.insert("info", info);

	// setup common
	log["time"] = GetDateTime().c_str();
	log["timezone"] = "Asia/Seoul";
	log["logid"] = MAP_LOG_ID_INSTALL;
	log["seq"] = GetTransactionID();
	user_s["AppGroupCode"] = m_appGroupCode.c_str();
	log.insert("user_s", user_s);

	logArray.append(log);

	root.insert("log", logArray);

	QJsonDocument doc(root);
	string json = doc.toJson().toStdString();

	LOG(DEBUG) << json.c_str();

	PostLog(json);

	return true;
}

bool MAPManager::LogStart()
{
	if (m_login == false)
		return false;

	QJsonObject root;
	QJsonObject log;
	QJsonObject command;
	QJsonObject info;
	QJsonObject user_s;
	QJsonArray logArray;

	// setup command 
	command["adid"] = m_adid.c_str();
	log.insert("start", command);

	// setup info
	info["os"] = "NGP_WIN";
	info["osver"] = QString::fromLocal8Bit(m_osInfo.c_str());
	info["did"] = m_uuid.c_str();
	info["md"] = m_cpuInfo.c_str();
	info["ctver"] = m_version.c_str();
	info["country"] = m_locale.c_str();
	info["store"] = "ncsoft";
	info["uid"] = m_user_id.c_str();
	info["oauth"] = "np";
	info["appid"] = m_appID.c_str();
	info["ug1"] = "";
	log.insert("info", info);

	// setup common
	log["time"] = GetDateTime().c_str();
	log["timezone"] = "Asia/Seoul";
	log["logid"] = MAP_LOG_ID_START;
	log["seq"] = GetTransactionID();
	user_s["AppGroupCode"] = m_appGroupCode.c_str();
	log.insert("user_s", user_s);

	logArray.append(log);

	root.insert("log", logArray);

	QJsonDocument doc(root);
	string json = doc.toJson().toStdString();

	LOG(DEBUG) << json.c_str();

	PostLog(json);

	StartKeepAliveTimer();

	return true;
}

bool MAPManager::LogEnd()
{
	if (m_login == false)
		return false;

	QJsonObject root;
	QJsonObject log;
	QJsonObject command;
	QJsonObject info;
	QJsonObject user_s;
	QJsonArray logArray;

	// setup command 
	log.insert("end", command);

	// setup info
	info["os"] = "NGP_WIN";
	info["osver"] = QString::fromLocal8Bit(m_osInfo.c_str());
	info["did"] = m_uuid.c_str();
	info["md"] = m_cpuInfo.c_str();
	info["ctver"] = m_version.c_str();
	info["country"] = m_locale.c_str();
	info["store"] = "ncsoft";
	info["uid"] = m_user_id.c_str();
	info["oauth"] = "np";
	info["appid"] = m_appID.c_str();
	info["ug1"] = "";
	log.insert("info", info);

	// setup common
	log["time"] = GetDateTime().c_str();
	log["timezone"] = "Asia/Seoul";
	log["logid"] = MAP_LOG_ID_END;
	log["seq"] = GetTransactionID();
	user_s["AppGroupCode"] = m_appGroupCode.c_str();
	log.insert("user_s", user_s);

	logArray.append(log);

	root.insert("log", logArray);

	QJsonDocument doc(root);
	string json = doc.toJson().toStdString();

	LOG(DEBUG) << json.c_str();

	PostLogSync(json);

	StopKeepAliveTimer();

	m_login = false;

	return true;
}

bool MAPManager::LogStartStream(bool useCam, bool useMic)
{
	if (m_login == false)
		return false;

	QJsonObject root;
	QJsonObject log;
	QJsonObject command;
	QJsonObject info;
	QJsonObject user_s;
	QJsonArray logArray;

	// setup command 
	command["nm"] = "start_stream";
	command["category"] = "start";
	command["param1"] = useCam == true ? "1" : "0";
	command["param2"] = useMic == true ? "1" : "0";
	log.insert("custom", command);

	// setup info
	info["os"] = "NGP_WIN";
	info["osver"] = QString::fromLocal8Bit(m_osInfo.c_str());
	info["did"] = m_uuid.c_str();
	info["md"] = m_cpuInfo.c_str();
	info["ctver"] = m_version.c_str();
	info["country"] = m_locale.c_str();
	info["store"] = "ncsoft";
	info["uid"] = m_user_id.c_str();
	info["oauth"] = "np";
	info["appid"] = m_appID.c_str();
	info["ug1"] = "";
	log.insert("info", info);

	// setup common
	log["time"] = GetDateTime().c_str();
	log["timezone"] = "Asia/Seoul";
	log["logid"] = MAP_LOG_ID_CUSTOM;
	log["seq"] = GetTransactionID();
	user_s["AppGroupCode"] = m_appGroupCode.c_str();
	log.insert("user_s", user_s);

	logArray.append(log);

	root.insert("log", logArray);

	QJsonDocument doc(root);
	string json = doc.toJson().toStdString();

	LOG(DEBUG) << json.c_str();

	PostLog(json);

	return true;
}

bool MAPManager::LogStopStream(bool useCam, bool useMic)
{
	if (m_login == false)
		return false;

	QJsonObject root;
	QJsonObject log;
	QJsonObject command;
	QJsonObject info;
	QJsonObject user_s;
	QJsonArray logArray;

	// setup command 
	command["nm"] = "end_stream";
	command["category"] = "end";
	command["param1"] = useCam == true ? "1" : "0";
	command["param2"] = useMic == true ? "1" : "0";
	log.insert("custom", command);

	// setup info
	info["os"] = "NGP_WIN";
	info["osver"] = QString::fromLocal8Bit(m_osInfo.c_str());
	info["did"] = m_uuid.c_str();
	info["md"] = m_cpuInfo.c_str();
	info["ctver"] = m_version.c_str();
	info["country"] = m_locale.c_str();
	info["store"] = "ncsoft";
	info["uid"] = m_user_id.c_str();
	info["oauth"] = "np";
	info["appid"] = m_appID.c_str();
	info["ug1"] = "";
	log.insert("info", info);

	// setup common
	log["time"] = GetDateTime().c_str();
	log["timezone"] = "Asia/Seoul";
	log["logid"] = MAP_LOG_ID_CUSTOM;
	log["seq"] = GetTransactionID();
	user_s["AppGroupCode"] = m_appGroupCode.c_str();
	log.insert("user_s", user_s);

	logArray.append(log);

	root.insert("log", logArray);

	QJsonDocument doc(root);
	string json = doc.toJson().toStdString();

	LOG(DEBUG) << json.c_str();

	PostLog(json);

	return true;
}

bool MAPManager::LogStatusMic(bool active, const string& deviceName)
{
	if (m_login == false)
		return false;

	QJsonObject root;
	QJsonObject log;
	QJsonObject command;
	QJsonObject info;
	QJsonObject user_s;
	QJsonArray logArray;

	// setup command 
	command["nm"] = "mic_status";
	command["category"] = "mic";
	command["param1"] = active == true ? "1" : "0";
	command["param2"] = QString::fromLocal8Bit(deviceName.c_str());
	log.insert("custom", command);

	// setup info
	info["os"] = "NGP_WIN";
	info["osver"] = QString::fromLocal8Bit(m_osInfo.c_str());
	info["did"] = m_uuid.c_str();
	info["md"] = m_cpuInfo.c_str();
	info["ctver"] = m_version.c_str();
	info["country"] = m_locale.c_str();
	info["store"] = "ncsoft";
	info["uid"] = m_user_id.c_str();
	info["oauth"] = "np";
	info["appid"] = m_appID.c_str();
	info["ug1"] = "";
	log.insert("info", info);

	// setup common
	log["time"] = GetDateTime().c_str();
	log["timezone"] = "Asia/Seoul";
	log["logid"] = MAP_LOG_ID_CUSTOM;
	log["seq"] = GetTransactionID();
	user_s["AppGroupCode"] = m_appGroupCode.c_str();
	log.insert("user_s", user_s);

	logArray.append(log);

	root.insert("log", logArray);

	QJsonDocument doc(root);
	string json = doc.toJson().toStdString();

	LOG(DEBUG) << json.c_str();

	PostLog(json);

	return true;
}

bool MAPManager::LogStatusCam(bool active, const string& deviceName)
{
	if (m_login == false)
		return false;

	QJsonObject root;
	QJsonObject log;
	QJsonObject command;
	QJsonObject info;
	QJsonObject user_s;
	QJsonArray logArray;

	// setup command 
	command["nm"] = "camera_status";
	command["category"] = "camera";
	command["param1"] = active == true ? "1" : "0";
	command["param2"] = QString::fromLocal8Bit(deviceName.c_str());
	log.insert("custom", command);

	// setup info
	info["os"] = "NGP_WIN";
	info["osver"] = QString::fromLocal8Bit(m_osInfo.c_str());
	info["did"] = m_uuid.c_str();
	info["md"] = m_cpuInfo.c_str();
	info["ctver"] = m_version.c_str();
	info["country"] = m_locale.c_str();
	info["store"] = "ncsoft";
	info["uid"] = m_user_id.c_str();
	info["oauth"] = "np";
	info["appid"] = m_appID.c_str();
	info["ug1"] = "";
	log.insert("info", info);

	// setup common
	log["time"] = GetDateTime().c_str();
	log["timezone"] = "Asia/Seoul";
	log["logid"] = MAP_LOG_ID_CUSTOM;
	log["seq"] = GetTransactionID();
	user_s["AppGroupCode"] = m_appGroupCode.c_str();
	log.insert("user_s", user_s);

	logArray.append(log);

	root.insert("log", logArray);

	QJsonDocument doc(root);
	string json = doc.toJson().toStdString();

	LOG(DEBUG) << json.c_str();

	PostLog(json);

	return true;
}

bool MAPManager::LogAddCaption(const string& caption)
{
	if (m_login == false)
		return false;

	QJsonObject root;
	QJsonObject log;
	QJsonObject command;
	QJsonObject info;
	QJsonObject user_s;
	QJsonArray logArray;

	// setup command 
	command["nm"] = "subtitles";
	command["category"] = "overlay";
	command["param1"] = to_string(caption.length()).c_str();
	log.insert("custom", command);

	// setup info
	info["os"] = "NGP_WIN";
	info["osver"] = QString::fromLocal8Bit(m_osInfo.c_str());
	info["did"] = m_uuid.c_str();
	info["md"] = m_cpuInfo.c_str();
	info["ctver"] = m_version.c_str();
	info["country"] = m_locale.c_str();
	info["store"] = "ncsoft";
	info["uid"] = m_user_id.c_str();
	info["oauth"] = "np";
	info["appid"] = m_appID.c_str();
	info["ug1"] = "";
	log.insert("info", info);

	// setup common
	log["time"] = GetDateTime().c_str();
	log["timezone"] = "Asia/Seoul";
	log["logid"] = MAP_LOG_ID_CUSTOM;
	log["seq"] = GetTransactionID();
	user_s["AppGroupCode"] = m_appGroupCode.c_str();
	log.insert("user_s", user_s);

	logArray.append(log);

	root.insert("log", logArray);

	QJsonDocument doc(root);
	string json = doc.toJson().toStdString();

	LOG(DEBUG) << json.c_str();

	PostLog(json);

	return true;
}

bool MAPManager::LogKeepAlive()
{
	if (m_login == false)
		return false;

	QJsonObject root;
	QJsonObject log;
	QJsonObject command;
	QJsonObject info;
	QJsonObject hb;
	QJsonArray logArray;

	// setup command 
	command["adid"] = m_adid.c_str();
	log.insert("start", command);

	// setup info
	info["os"] = "NGP_WIN";
	info["osver"] = QString::fromLocal8Bit(m_osInfo.c_str());
	info["did"] = m_uuid.c_str();
	info["md"] = m_cpuInfo.c_str();
	info["ctver"] = m_version.c_str();
	info["country"] = m_locale.c_str();
	info["store"] = "ncsoft";
	info["uid"] = m_user_id.c_str();
	info["oauth"] = "np";
	info["appid"] = m_appID.c_str();
	info["ug1"] = "";
	log.insert("info", info);

	// setup common
	log["time"] = GetDateTime().c_str();
	log["timezone"] = "Asia/Seoul";
	log["logid"] = MAP_LOG_ID_START;
	log["seq"] = GetTransactionID();
	hb["hb"] = "1";
	log.insert("user_s", hb);

	logArray.append(log);

	root.insert("log", logArray);

	QJsonDocument doc(root);
	string json = doc.toJson().toStdString();

	LOG(DEBUG) << json.c_str();

	PostLog(json);

	return true;
}

bool MAPManager::PostLog(const string& msg)
{
	QByteArray msgBody(msg.c_str(), (int)msg.length());

	// generate JWT =====================================================
	string user_agent = format_string("PurpleStudio (%s/%s)", m_appID.c_str(), m_version.c_str());

	auto token = jwt::create()
		.set_type("JWT")
		.set_algorithm("RS256")
		.set_payload_claim("iss", jwt::claim(m_appID))
		.set_payload_claim("iat", jwt::claim(std::chrono::system_clock::now()))
		.set_payload_claim("typ", jwt::claim(std::string("session")))
		.set_payload_claim("user-agent", jwt::claim(user_agent))
		.set_payload_claim("access_token", jwt::claim(m_session_id))
		.sign(jwt::algorithm::rs256{ "", rsa_priv_key, "", "***********" });

	string strURL = format_string("%s/logs/v1.0/%s/logs", GetApiGateURL(App()->GetServiceNetwork()), m_appID.c_str());
	QUrl url = QString::fromUtf8(strURL.c_str());
	QNetworkRequest request(url);

	request.setRawHeader("Accept", "application/json");
	request.setRawHeader("Connection", "keep-alive");
	request.setRawHeader("Content-Length", format_string("%d", msg.length()).c_str());
	request.setRawHeader("Content-Type", "application/json; charset=utf-8");
	request.setRawHeader("Authorization", format_string("JWT %s", token.c_str()).c_str());

	debugRequest(request, msgBody);
	QNetworkReply* reply = m_pManager->post(request, msgBody);
	QObject::connect(reply, &QNetworkReply::finished, [=]() 
	{
		if (reply->error() == QNetworkReply::NoError)
		{
			QByteArray response = reply->readAll();
			LOG(DEBUG) << "OK:" << response.toStdString();
		}
		else // handle error
		{
			OnError(reply);
		}
	});

	return true;
}

bool MAPManager::waitForConnect(int nTimeOutms, QNetworkReply* pReply)
{
	QTimer *timer = NULL;
	QEventLoop eventLoop;
	bool bReadTimeOut = false;
	m_EndLogTimeout = false;
	if (nTimeOutms > 0)
	{
		timer = new QTimer(this);

		connect(timer, SIGNAL(timeout()), this, SLOT(slotWaitTimeout()));
		timer->setSingleShot(true);
		timer->start(nTimeOutms);

		connect(this, SIGNAL(signalReadTimeout()), &eventLoop, SLOT(quit()));
	}

	// Wait on QNetworkManager reply here
	connect(m_pManager, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));

	if (pReply != NULL)
	{
		// Preferrably we wait for the first reply which comes faster than the finished signal
		connect(pReply, SIGNAL(readyRead()), &eventLoop, SLOT(quit()));
	}
	eventLoop.exec();

	if (timer != NULL)
	{
		timer->stop();
		delete timer;
		timer = NULL;
	}

	bReadTimeOut = m_EndLogTimeout;
	m_EndLogTimeout = false;

	return !bReadTimeOut;
}

void MAPManager::slotWaitTimeout()
{
	m_EndLogTimeout = true;  // Report timeout
}

bool MAPManager::PostLogSync(const string& msg)
{
	QByteArray msgBody(msg.c_str(), (int)msg.length());

	// generate JWT =====================================================
	string user_agent = format_string("PurpleStudio (%s/%s)", m_appID.c_str(), m_version.c_str());

	auto token = jwt::create()
		.set_type("JWT")
		.set_algorithm("RS256")
		.set_payload_claim("iss", jwt::claim(m_appID))
		.set_payload_claim("iat", jwt::claim(std::chrono::system_clock::now()))
		.set_payload_claim("typ", jwt::claim(std::string("session")))
		.set_payload_claim("user-agent", jwt::claim(user_agent))
		.set_payload_claim("access_token", jwt::claim(m_session_id))
		.sign(jwt::algorithm::rs256{ "", rsa_priv_key, "", "***********" });

	string strURL = format_string("%s/logs/v1.0/%s/logs", GetApiGateURL(App()->GetServiceNetwork()), m_appID.c_str());
	//string strURL = format_string("%s/logs/v1.0/%s/logs", get_api_gate(m_sn).c_str(), m_appID.c_str());
	QUrl url = QString::fromUtf8(strURL.c_str());
	QNetworkRequest request(url);

	request.setRawHeader("Accept", "application/json");
	request.setRawHeader("Connection", "keep-alive");
	request.setRawHeader("Content-Length", format_string("%d", msg.length()).c_str());
	request.setRawHeader("Content-Type", "application/json; charset=utf-8");
	request.setRawHeader("Authorization", format_string("JWT %s", token.c_str()).c_str());

	debugRequest(request, msgBody);
	QNetworkReply* reply = m_pManager->post(request, msgBody);

	if (!waitForConnect(1000, reply))
	{
		// timeout
		LOG(DEBUG) << "SendLog Timeout!!!";
		return false;
	}

	if (reply->error() == QNetworkReply::NoError)
	{
		QByteArray response = reply->readAll();
		LOG(DEBUG) << "OK:" << response.toStdString();
	}
	else // handle error
	{
		OnError(reply);
		return false;
	}

	return true;
}

void CALLBACK MAPManager::KeepAliveProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	Q_UNUSED(uTimerID);
	Q_UNUSED(uMsg);
	Q_UNUSED(dw1);
	Q_UNUSED(dw2);

	QMetaObject::invokeMethod((MAPManager*)(dwUser), "LogKeepAlive");
}

void MAPManager::StartKeepAliveTimer()
{
	StopKeepAliveTimer();

	m_nTimerID = timeSetEvent((UINT)m_nKeepAliveIntervalms, 0, this->KeepAliveProc, (DWORD_PTR)this, TIME_PERIODIC);
}

void MAPManager::StopKeepAliveTimer()
{
	if (m_nTimerID != 0)
	{
		timeKillEvent(m_nTimerID);
	}
}

#pragma comment(lib, "rpcrt4.lib")  // UuidCreate - Minimum supported OS Win 2000
string MAPManager::MakeUUID()
{
	UUID uuid;
	UuidCreate(&uuid);
	char *str;
	UuidToStringA(&uuid, (RPC_CSTR*)&str);
	string _uuid = str;
	RpcStringFreeA((RPC_CSTR*)&str);

	return _uuid;
}

void MAPManager::GetSystemInfo()
{
	// locale
	GEOID myGEO = GetUserGeoID(GEOCLASS_NATION);
	int sizeOfBuffer = GetGeoInfo(myGEO, GEO_ISO2, NULL, 0, 0);
	WCHAR *buffer = new WCHAR[sizeOfBuffer];
	int result = GetGeoInfo(myGEO, GEO_ISO2, buffer, sizeOfBuffer, 0);

	std::wstring locale_w = buffer;
	m_locale.assign(locale_w.begin(), locale_w.end());
	delete [] buffer;

	// adid
	IP_ADAPTER_INFO* info = new IP_ADAPTER_INFO;
	unsigned long size = sizeof(IP_ADAPTER_INFO);

	result = GetAdaptersInfo(info, &size);        // 첫번째 랜카드 MAC address 가져오기
	if (result != ERROR_BUFFER_OVERFLOW)
	{
		string strMac = format_string("%0.2X%0.2X%0.2X%0.2X%0.2X%0.2X",
			info->Address[0], info->Address[1],
			info->Address[2], info->Address[3],
			info->Address[4], info->Address[5]);

		m_adid = QString(QCryptographicHash::hash((strMac.c_str()), QCryptographicHash::Md5).toHex()).toStdString();
	}
	else
		m_adid = "";


	// system info
	SystemInfo::GetSystemInfo(m_cpuInfo, m_osInfo);
}

string MAPManager::GetDateTime()
{
	struct timeb timer_msec;
	ftime(&timer_msec);

	tm *ts = localtime(&timer_msec.time);

	string now = format_string("%04d-%02d-%02d %02d:%02d:%02d.%d", ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday, ts->tm_hour, ts->tm_min, ts->tm_sec, timer_msec.millitm);
	return now;
}

long MAPManager::GetTransactionID()
{
	InterlockedIncrement(&m_tid);
	return m_tid;
}

void MAPManager::OnError(QNetworkReply* reply)
{
	QByteArray response = reply->readAll();
	LOG(DEBUG) << "Error:" << reply->errorString().toStdString();
	QString str = QTextCodec::codecForMib(106)->toUnicode(response);
	LOG(DEBUG) << str.toLocal8Bit().toStdString().c_str();

	QJsonDocument body(QJsonDocument::fromJson(response));
	QJsonObject root = body.object();
	int errorCode = root["error"].toInt();
	if (errorCode == 3325)	//NET_ERR_SESSION_PERSISTENT
	{
		//refresh session
	}
}

const string MAPManager::GetApiGateURL(eServiceNetwork sn)
{
	switch (sn)
	{
	case eSN_RC:
		return API_GATE_RC;

	case eSN_OP:
		assert(0);
		return API_GATE_OP;

	case eSN_SB:
		return API_GATE_SANDBOX;

	default:
	case eSN_LIVE:
		return API_GATE_LIVE;
	}
}
