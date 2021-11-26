#include "MainWindow.h"

#include <qtimer.h>
#include <qsettings.h>
#include <qstandardpaths.h>
#include <QMessageBox>
#include <qscreen.h>
#include <qwindow.h>
#include <qbitarray.h>
#include <qevent.h>
#include <qpainter.h>
#include <QClipboard>
#include <qmovie.h>

#include "LocalSettings.h"
#include "HiddenSettings.h"
#include "NCCaptureSDK.h"
#include "VideoChatApp.h"
#include "VideoChatMessageBox.h"
#include "CommunityLiveManager.h"
#include "AppUtils.h"
#include "AppString.h"
#include "easylogging++.h"
#include "VideoChatApp.h"
#include "AudioMixer/NCAudioMixer.h"
#include "MenuControl.h"
#include "MAPManager.h"
#include "MaskingManager.h"

MainWindow::MainWindow(QWidget *parent)
    : QFramelessDialog(parent, true, 5, true, 0.0)
{
	LOG(DEBUG) << format_string("[MainWindow::MainWindow]");

    ui.setupUi(this);

	InitUI();

	m_startUpWindow = nullptr;
	m_startUpLauncherWindow = nullptr;
	m_viewerlistpopup = nullptr;

	CreateStartUpGameWindow();
	CreateStartUpLauncherWindow();
	CreatePopUp();
}

MainWindow::~MainWindow()
{
	if (m_startUpWindow)
		delete m_startUpWindow;
	if (m_startUpLauncherWindow)
		delete m_startUpLauncherWindow;
	if (m_p_streamTimer)
		delete m_p_streamTimer;
	if (m_viewerlistpopup)
		delete m_viewerlistpopup;

	LOG(DEBUG) << format_string("[MainWindow::~MainWindow]");
}

void MainWindow::InitUI()
{
	ui.time_label->setText("00:00:00");
	ui.time_label->setStyleSheet("color: rgba(232, 234, 255, 0.5);");
	ui.live_icon_btn->setDisabled(true);
	ui.viewer_list_icon_btn->setText("");

	this->setWindowIcon(QIcon(QString::fromStdString(AppConfig::Instance()->purplelive_icon_path)));
	m_streamTimerFont = QFont(AppUtils::GetRegularFont(14));

	// install event filter
	ui.min_btn->installEventFilter(this);
	ui.close_btn->installEventFilter(this);
	ui.time_label->installEventFilter(this);
	ui.roomName_label->installEventFilter(this);

	ui.pushButton_share->SetIcon(QPixmap(":/MainWindow/image/share.svg"));
	ui.pushButton_setting->SetIcon(QPixmap(":/MainWindow/image/setting.svg"));

	ui.spacer_widget->hide();
	setViewerCount(0);
	SetEraseBackGround(m_backGroundColor);

	hide();
}

void MainWindow::InitUIAfterCreation()
{
	UpdateLanguageString();
	ui.widget_preview->Init();
}

MainWindow * MainWindow::Get()
{
	return reinterpret_cast<MainWindow*>(App()->GetMainWindow());
}

QWidget* MainWindow::GetActiveWidget()
{
	if (MainWindow::Get()->m_startUpWindow == nullptr)
		return MainWindow::Get();

	if (MainWindow::Get()->m_startUpWindow->isHidden())
	{
		return MainWindow::Get();
	}
	else {
		return MainWindow::Get()->m_startUpWindow;
	}
}

void MainWindow::setViewerCount(const int _viewernum)
{
	if (ui.viewer_list_icon_btn->SetViewerCount(_viewernum))
	{
		repaint();
	}
}

bool MainWindow::GetActive()
{
	return m_active;
}

void MainWindow::SetActive(bool enable)
{
	m_active = enable;
	if (enable)
	{
		show();
		CheckHideInfoBox();
	}
	else
	{
		hide();
	}
}

bool MainWindow::Init()
{
	QRect default_rect = AppUtils::GetScreenCenterGeometry(this, minimumWidth(), minimumHeight());	
	QRect saved_rect = LocalSettings::GetWindowGeometry(metaObject()->className(), default_rect);
	if (!AppUtils::VerifyWindowRect(this, saved_rect))
	{
		saved_rect = default_rect;
	}
	setGeometry(saved_rect);

	bool isMaximized = false;
	isMaximized = LocalSettings::GetWindowMaximized(metaObject()->className(), isMaximized);

	if (!isMaximized)
	{
		setWindowState(Qt::WindowNoState);
	}
	else
	{
		setWindowState(Qt::WindowMaximized);
	}

	// set default language
	App()->SetLocalLanguage(App()->GetLocalLanguage());

	QTimer* pTimer = new QTimer(this);
	connect(pTimer, &QTimer::timeout, ui.widget_preview, &PreviewWidget::animate);
	pTimer->start(50);
	
	hide();

	if (App()->GetForceExcute())
	{
		if (StartRender(App()->GetForceExcute(), App()->GetRoomID().c_str()))
		{
			SetActive(true);
		}
	}
	else
	{
		if (!(SetActiveStartUpGameWindow(true)))
			return false;
	}

	InitUIAfterCreation();

	AppUtils::DispatchToMainThread([]() {
		MainWindow::Get()->BringToFront();
	});

	return true;
}

void AudioCaptureCallback(audio_frame_data* frame, void* user_data)
{
	audio_data* adata = CommunityLiveManager::Instance()->GetAudioData();
	adata->data[0] = frame->data;
	adata->frames = frame->frames;
	CommunityLiveManager::Instance()->OnAudioFrame(adata);		
}

void AudioMagnitudeCallback(audio_magnitude* info, void* user_data)
{
	if (info->is_input)
	{
		MainWindow* pMainWindow = (MainWindow*)user_data;
		pMainWindow->SetMicInputLevel(info->magnitude_rate);

		MenuControl::Instance()->SetMicMagnitude(info->magnitude_rate);
	}
}

void MainWindow::SetMicInputLevel(float micInput)
{
	
}

#ifdef _DEBUG
void MainWindow::keyPressEvent(QKeyEvent * event)
{
	if (event->key() == Qt::Key_Q)
	{
		VideoChatMessageBox::Instance()->question(AppString::GetString("VideoChatMessageBox/info_copy_link_to_clipboard"));
	}
	if (event->key() == Qt::Key_A)
	{
		VideoChatMessageBox::Instance()->information(AppString::GetString("VideoChatMessageBox/info_copy_link_to_clipboard"));
	}
}
#endif//_DEBUG

void MainWindow::SetStreamingSize(int width, int height)
{
	LOG(DEBUG) << format_string("[MainWindow::SetStreamingSize] (%d %d)", width, height);
	AppConfig::Instance()->streaming_width = width;
	AppConfig::Instance()->streaming_height = height;
	MainWindow::Get()->PreviewResize();
}

void MainWindow::SetTarget(HWND hwnd)
{
	LOG(DEBUG) << format_string("[MainWindow::SetTarget] hwnd(0x%d)", hwnd);
	m_targetHwnd = hwnd;
}

void MainWindow::SetChangeStreamingStateCallback(std::function<void (eStreamingState streamingState)> callback)
{
	m_streamStateCallback = callback;
}

void MainWindow::StopRender()
{
	LOG(INFO) << format_string("[MainWindow::StopRender]");

	NCAudioMixer::Instance()->Stop();
	NCCapture::Instance()->StopCapture();
	ui.widget_preview->StopRender();
}

bool MainWindow::StartRender(bool isForceExcute, const char* strRoomID, const char* strRoomName, const char* strGroupUserID, const char* strChannelID)
{
	LOG(INFO) << format_string("[MainWindow::StartRender]");

	LimeCommunicator::Instance()->SetStreamingState(eStreamingState::eSS_Starting);

	if (m_targetHwnd==nullptr || !IsWindow(m_targetHwnd))
	{
		LOG(ERROR) << format_string("[MainWindow::StartRender] [error] targetHwnd(0x%p) IsWindow(%d)", m_targetHwnd, IsWindow(m_targetHwnd));
		LimeCommunicator::Instance()->SetStreamingState(eStreamingState::eSS_Error);
		AppClose(AppString::GetString("error_message/error_target_game").c_str());
		return false;
	}

	CommunityLiveManager::Instance();

	SetAudioOutput();
	NCAudioMixer::Instance()->RegisterCaptureCallback(AudioCaptureCallback, (void*)this);
	NCAudioMixer::Instance()->RegisterMagnitudeCallback(AudioMagnitudeCallback, (void*)this);
	NCAudioMixer::Instance()->SetEnableMeasureMagnitude(true, true);
	NCAudioMixer::Instance()->Start();
	if (!NCCapture::Instance()->StartCapture(m_targetHwnd))
	{
		LOG(ERROR) << format_string("[MainWindow::StartRender] [error] StartCapture failed targetHwnd(0x%p)", m_targetHwnd);
		LimeCommunicator::Instance()->SetStreamingState(eStreamingState::eSS_Error);
		AppClose(AppString::GetString("error_message/error_game_capture").c_str());
		return false;
	}
	ui.widget_preview->StartRender(App()->GetOriginGameCode().c_str());

	LimeCommunicator::Instance()->SetForceExcute(isForceExcute);
	LimeCommunicator::Instance()->SetRoomID(strRoomID);
	LimeCommunicator::Instance()->SetRoomName(strRoomName);
	LimeCommunicator::Instance()->SetShareTarget(strGroupUserID, strChannelID);

	if (LimeCommunicator::Instance()->GetCharactorList(App()->GetCharacterCode().c_str(), App()->GetToken().c_str(), App()->GetServiceNetwork(), App()->GetAppVersion().c_str()))
	{
		RequestStartStreaming();
		return true;
	}

	LimeCommunicator::Instance()->SetStreamingState(eStreamingState::eSS_Error);
	return false;
}

void MainWindow::BringToFront()
{
	LOG(INFO) << format_string("[MainWindow::BringToFront]");
	
	QWidget* widget = MainWindow::GetActiveWidget();

	// minimized to active
	widget->setWindowState((widget->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);

	SetWindowPos((HWND)widget->winId(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	SetWindowPos((HWND)widget->winId(), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	widget->setFocus();
	widget->activateWindow();
	widget->raise();
}

void MainWindow::RetryExcution()
{
	LOG(INFO) << format_string("[MainWindow::RetryExcution]");
	VideoChatMessageBox::Instance()->information_stimer(AppString::GetString("event_message/event_retry_excution"), {}, 5);
	AppClose();
}

QString MainWindow::GetMicNameFromIndex(int index)
{
	if (m_vecMicInfoList.size() > index) {
		return QString::fromLocal8Bit(m_vecMicInfoList[index].name.c_str());
	}
	else {
		return AppString::GetQString("Common/un_used");
	}
}

void MainWindow::GetMicInfoList(std::vector<AudioDeviceInfo>& mic_list)
{
	mic_list = NCAudioMixer::Instance()->GetDeviceInfoList(1);

	if (this != nullptr) {
		m_vecMicInfoList.clear();
		m_vecMicInfoList.assign(mic_list.begin(), mic_list.end());
	}
}

void MainWindow::SetLocalSavedMicSetting()
{
	bool micMuted = LocalSettings::Instance()->GetMicMute(AppConfig::default_mute);
	bool enableMicInputCapture = NCAudioMixer::Instance()->GetEnableInputCapture();
	SetMuted(micMuted, !enableMicInputCapture);
}

void MainWindow::SetMuted(bool muted, bool disable)
{
	ui.mute_pushButton->SetMuted(muted, disable);
	NCAudioMixer::Instance()->SetMute(true, muted);
}

void MainWindow::on_mute_pushButton_clicked()
{
	bool mute = LocalSettings::Instance()->GetMicMute(AppConfig::default_mute);

	SetMuted(!mute);

	LocalSettings::Instance()->SetMicMute(!mute);
}

void MainWindow::SetMicSelect(int index)
{
	SetMicIndex(index);

	// disable all audio input 
	NCAudioMixer::Instance()->SetEnableAudioDeviceAll(false, 1);
	
	// enable default audio input
	NCAudioMixer::Instance()->SetEnableAudioDevice(true, 1, index);
	
	if (NCAudioMixer::Instance()->IsMixing())
	{
		// start audio mixing
		NCAudioMixer::Instance()->Start();
	}
}

void MainWindow::SetAudioOutput()
{
	// disable all audio output 
	NCAudioMixer::Instance()->SetEnableAudioDeviceAll(false, 0);

	// enable default audio output
	NCAudioMixer::Instance()->SetEnableAudioDevice(true, 0, NCAudioMixer::DEFUALT_DEVICE_INDEX);	
	
	if (NCAudioMixer::Instance()->IsMixing())
	{
		// start audio mixing
		NCAudioMixer::Instance()->Start();
	}
}

float MainWindow::GetMicVolume()
{
	return NCAudioMixer::Instance()->GetInputCaptureVolume();
}

void MainWindow::SetEnableInputAudioCapture(bool enable)
{
	NCAudioMixer::Instance()->SetEnableInputCapture(enable);
}

void MainWindow::SetMicVolume(float volume)
{
	LocalSettings::SetMicVolume(volume);
	return NCAudioMixer::Instance()->SetInputCaptureVolume(volume);
}

void MainWindow::SetBossHide(bool enable)
{
	if (enable)
	{
#ifdef _DEBUG
		ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
		if (GetActive())
			hide();
		if (m_startUpWindow && m_startUpWindow->GetActive())
			m_startUpWindow->hide();
		if (m_startUpLauncherWindow && m_startUpLauncherWindow->GetActive())
			m_startUpLauncherWindow->hide();
	}
	else
	{
#ifdef _DEBUG
		ShowWindow(GetConsoleWindow(), SW_SHOW);
#endif
		if (GetActive())
			show();
		if (m_startUpWindow && m_startUpWindow->GetActive())
			m_startUpWindow->show();
		if (m_startUpLauncherWindow && m_startUpLauncherWindow->GetActive())
			m_startUpLauncherWindow->show();
	}
}

void MainWindow::SetScreenLock(bool enable)
{
	if(enable) ScreenLockWidget::Instance()->Show();
	else  ScreenLockWidget::Instance()->Hide();
}

void MainWindow::AppClose(const char* message)
{
	if (m_closing)
		return;

	m_closing = true;
	m_closingMessage = (bool)message;

	MAPManager::Instance()->LogEnd();

	StopStreaming();

	bool streaming = LimeCommunicator::Instance()->GetStreamingState() == eStreamingState::eSS_Streaming;
	if (streaming)
	{
		RequestStopStreaming();
	}

	if (message)
	{
		LOG(INFO) << "[MainWindow::AppClose] message : \n" << StringUtils::GetStringFromUtf8(message);
		VideoChatMessageBox::Instance()->information(message);
		m_closingMessage = false;
	}

	if (m_closeRoomComplete || !streaming)
	{
		QApplication::quit();
	}
	else
	{
		QTimer* timer = new QTimer();
		timer->setSingleShot(true);
		timer->start(AppConfig::LIME_CLOSE_ROOM_TIMEOUT);
		QObject::connect(timer, &QTimer::timeout, [timer]() {
			elogi("[MainWindow::AppClose] LIME CloseRoom response waiting timeout");
			timer->deleteLater();
			QApplication::quit();
		});
	}
}

void MainWindow::OnLanguageChanged()
{
	if (MainWindow::Get()->GetStartupWindow() && !MainWindow::Get()->getStartUpWindowIsHidden())
	{
		MainWindow::Get()->GetStartupWindow()->OnLanguageChanged();
	}

	eStreamingState streamingState = LimeCommunicator::Instance()->GetStreamingState();
	if (streamingState == eStreamingState::eSS_Starting || streamingState == eStreamingState::eSS_Streaming)
	{
		MaskingManager::Instance()->Setup(App()->GetOriginGameCode());
		GetPreviewWidget()->InitMask(App()->GetOriginGameCode().c_str());
	}
}

void MainWindow::onStreamTimer() 
{
	if (m_stream_s++ >= 59) {
		m_stream_s = 0;
		if (m_stream_m++ >= 59) {
			m_stream_m = 0;
			m_stream_h++;
		}
	}

	if (m_currHourDigits < QString::number(m_stream_h).length()) {
		m_currHourDigits = QString::number(m_stream_h).length();
		ui.time_label->setFixedWidth(ui.time_label->width() + STREAM_TIMER_LETTER_WIDTH);
		ui.time_label->repaint();
	}
	else
	{
		ui.time_label->update();
	}
}

void MainWindow::CreateStartUpGameWindow()
{
	m_startUpWindow = new StartUpWindow(nullptr);
}

void MainWindow::CreateStartUpLauncherWindow()
{
	m_startUpLauncherWindow = new StartUpLauncherWindow(nullptr);
}

bool MainWindow::SetActiveStartUpGameWindow(bool is_active)
{
	if (is_active)
	{
		if (!(m_startUpWindow->Init()))
			return false;
	}
	else 
	{ 
		m_startUpWindow->SetActive(false);
	}

	return true;
}

void MainWindow::SetActiveStartUpLauncherWindow(bool is_active)
{
	m_startUpLauncherWindow->SetActive(is_active);
}

void MainWindow::RequestStartStreaming()
{
	LimeCommunicator::Instance()->RequestStartStreaming();
}

void MainWindow::RequestStopStreaming()
{
	LimeCommunicator::Instance()->RequestStopStreaming();
}

void MainWindow::RequestCloseRoom()
{
	LimeCommunicator::Instance()->RequestCloseRoom();
}

void MainWindow::RequestCreateChatting(const GuildUserInfoList& info)
{
	string gameUserID = LimeCommunicator::Instance()->GetGameUserID(App()->GetCharacterName().c_str());
	string params = LimeCommunicator::Instance()->GetCreateChattingParams(gameUserID.c_str(), info);
	LimeCommunicator::Instance()->RequestCreateChatting(params.c_str());
}

bool MainWindow::RequestGuildUserList()
{
	string gameUserID = LimeCommunicator::Instance()->GetGameUserID(App()->GetCharacterName().c_str());
	LimeCommunicator::Instance()->RequestGuildUserList();
	return true;
}

void MainWindow::RequestShareTargetList()
{
	string gameUserID = LimeCommunicator::Instance()->GetGameUserID(App()->GetCharacterName().c_str());
	LimeCommunicator::Instance()->RequestShareTargetList();
}

void MainWindow::RequestRoomUserList(bool isAppend, int count)
{
	LimeCommunicator::Instance()->RequestRoomUserList(isAppend, count);
}

void MainWindow::RequestWatchingUserList(bool isAppend, int count)
{
	LimeCommunicator::Instance()->RequestWatchingUserList(isAppend, count);
}

void MainWindow::RequestDeportRoom(const DeportUserInfo& info)
{
	LimeCommunicator::Instance()->RequestDeportRoom(info);
}

void MainWindow::StartStreaming(MediaSignalServerInfo& mediaSignalServerInfo, const char* strAuthKey)
{
	LOG(INFO) << format_string("[MainWindow::StartStreaming]");

	VideoEncodingType encoder_type = VET_SOFTWARE;
	int gpu_support = CommunityLiveManager::GpuDetect();
#if !USE_SOFTWARE_ENCODER_ONLY
	if (gpu_support & HES_NVIDIA)
		encoder_type = VET_HARDWARE_NVENC;
	else if (gpu_support & HES_AMF)
		encoder_type = VET_HARDWARE_AMF;
	else if (gpu_support & HES_QSV)
		encoder_type = VET_QSV11;
#endif

	LOG(INFO) << format_string("[MainWindow::StartStreaming] supported encoder nvidia(%d) amf(%d) qsv(%d), encoding_type(%d)", gpu_support & HES_NVIDIA, gpu_support & HES_AMF, gpu_support & HES_QSV, encoder_type);

	media_config media;
	media.framerate = AppConfig::Instance()->streaming_framerate;
	media.bitrate = AppConfig::Instance()->streaming_bitrate;
	media.encoding_type = encoder_type;
	CommunityLiveManager::Instance()->SetMediaConfig(media);

	std::string webSocketURL = mediaSignalServerInfo.webSocketUrl;
	std::string authKey = LimeCommunicator::Instance()->GetAuthKey();
	CommunityLiveManager::Instance()->MediaRoomJoin(webSocketURL.c_str(), authKey.c_str());
}

void MainWindow::StopStreaming()
{
	LOG(INFO) << format_string("[MainWindow::StopStreaming]");

	StopRender();

	CommunityLiveManager::Instance()->MediaRoomStop();

	SetStopStreamUI();
}

void MainWindow::SetStartStreamUI()
{
	ui.live_icon_btn->setDisabled(false);
	if (m_p_streamTimer == nullptr) {
		m_p_streamTimer = new QTimer(this);
		connect(m_p_streamTimer, SIGNAL(timeout()), this, SLOT(onStreamTimer()));
		m_p_streamTimer->start(1000);
	}

	ui.time_label->repaint();

	m_stream_s = 0;
	m_stream_m = 0;
	m_stream_h = 0;
}

void MainWindow::SetStopStreamUI()
{
	ui.live_icon_btn->setDisabled(true);
	if (m_p_streamTimer != nullptr) {
		m_p_streamTimer->stop();
		m_p_streamTimer->deleteLater();
		m_p_streamTimer = nullptr;
	}

	ui.time_label->repaint();
}

void MainWindow::OnPubServerConnected(bool reconnect)
{
	LOG(INFO) << format_string("[MainWindow::OnPubServerConnected] reconnect(%d)", reconnect);

	if (reconnect)
		return;

	if (m_startUpWindow)
		m_startUpWindow->ReloadCharactor();
}

void MainWindow::OnChangeStreamingState(eStreamingState streamingState)
{
	LOG(INFO) << format_string("[MainWindow::OnChangeStreamingState] streamingState(%d)", streamingState);

	switch (streamingState)
	{
		case eStreamingState::eSS_None:
			break;
		case eStreamingState::eSS_Starting:
			break;
		case eStreamingState::eSS_Streaming:
			SetStartStreamUI();
			RequestWatchingUserList();
			break;
		case eStreamingState::eSS_Stopping:
			break;
		case eStreamingState::eSS_Stopped:
			break;
		case eStreamingState::eSS_Error:
			break;
	}

	if (m_streamStateCallback)
		m_streamStateCallback(streamingState);
}

void MainWindow::OnCreateChatting(ChattingInfo& info, bool success)
{
	if (success)
	{
		elogd("[MainWindow::OnCreateChatting] =========================================");
		elogd("\tgroupId(%s) groupType(%s)", info.groupId.c_str(), info.groupType.c_str()); 
		elogd("[MainWindow::OnCreateChatting] =========================================");
	}

	m_startUpWindow->OnCreateChatting(success);
}

void MainWindow::OnGetGuildUserList(GuildUserInfoList& info, bool success)
{
	if (success)
	{
		elogd("[MainWindow::OnGetGuildUserList] =========================================");
		elogd("\tguildName(%s) gameCode(%s)", StringUtils::GetStringFromUtf8(info.guildName).c_str(), info.gameCode.c_str());
		for(int i=0; i<info.guildUserInfoList.size(); i++)
		{		
			elogd("\tgroupUserId(%s) gameUserId(%s), characterName(%s)", 
				info.guildUserInfoList[i].groupUserId.c_str(), info.guildUserInfoList[i].gameUserId.c_str(), StringUtils::GetStringFromUtf8(info.guildUserInfoList[i].characterName).c_str());
		}
		elogd("[MainWindow::OnGetGuildUserList] =========================================");
	}
	
	m_startUpWindow->OnGetGuildUserList(success);
}

void MainWindow::OnGetShareTargetList(vector<GroupInfo>& groupInfoList, bool success)
{
	if (success)
	{
		elogd("[MainWindow::OnGetShareTargetList] =========================================");
		for(int i=0; i<groupInfoList.size(); i++)
		{		
			elogd("\t[GROUP] groupId(%s) groupType(%s), gameChannelType(%s), name(%s) groupImageUrl.length(%d)", 
				groupInfoList[i].groupId.c_str(), groupInfoList[i].groupType.c_str(), 
				groupInfoList[i].gameChannelType.c_str(), StringUtils::GetStringFromUtf8(groupInfoList[i].name).c_str(), groupInfoList[i].groupImageUrl.length());
			for(int j=0; j<groupInfoList[i].shareChannelList.size(); j++)
			{
				elogd("\t\t[CHANNEL] groupUserId(%s) groupId(%s) channelId(%s), channelType(%s), name(%s)", 
					groupInfoList[i].shareChannelList[j].groupUserId.c_str(), 
					groupInfoList[i].shareChannelList[j].groupId.c_str(), groupInfoList[i].shareChannelList[j].channelId.c_str(),
					groupInfoList[i].shareChannelList[j].channelType.c_str(), StringUtils::GetStringFromUtf8(groupInfoList[i].shareChannelList[j].name).c_str());
			}
		}
		elogd("[MainWindow::OnGetShareTargetList] =========================================");
	}

	m_startUpWindow->OnGetShareTargetList(success);
}

void MainWindow::OnGetRoomUserList(StreamRoomUserInfoList& viewerList, bool success)
{
	
}

void MainWindow::OnGetWatchingUserList(StreamRoomUserInfoList& viewerList, bool success)
{
	if (viewerList.loaded_count < viewerList.total_count)
	{
		RequestWatchingUserList(true);
	}

	for (auto viewer : viewerList.viewerList)
	{
		m_viewerlistpopup->AddViewer(viewer);
	}

	setViewerCount(m_viewerlistpopup->getViewerCount());
	ResetGeoViewerPopUp();
}

void MainWindow::OnDeportRoom(bool success)
{
	elogd("[MainWindow::OnDeportRoom]");
}

void MainWindow::OnReconnectRoom(StreamRoomInfo& roomInfo, StreamRoomUserInfo& ownerInfo, bool success)
{
	elogi("[MainWindow::OnReconnectRoom] success(%d)", success);
	if (!success)
	{
		AppUtils::DispatchToMainThread([] {
			MainWindow::Get()->AppClose(AppString::GetString("event_message/error_lime_reconnect_room").c_str());
		});
	}
}

void MainWindow::OnCreateRoom(StreamRoomInfo& roomInfo, StreamRoomUserInfo& ownerInfo, bool success)
{
	elogi("[MainWindow::OnCreateRoom] success(%d)", success);
}

void MainWindow::OnJoinRoom(bool success)
{
	elogd("[MainWindow::OnJoinRoom] success(%d)", success);
}

void MainWindow::OnUpdateRoom(StreamRoomInfo& streamRoomInfo, bool success)
{
	elogd("[MainWindow::OnUpdateRoom]");
}

void MainWindow::OnStartStream(MediaSignalServerInfo& mediaSignalServerInfo, const char* strAuthKey, bool success)
{
	elogd("[MainWindow::OnStartStream] success(%d)", success);

	if (!success)
	{
		return;
	}

	StartStreaming(mediaSignalServerInfo, strAuthKey);
	
	// send to agent (start msg)
	App()->SendStreamStatusToAgent(true);
}

void MainWindow::OnStopStream(bool success)
{
	elogi("[MainWindow::OnStopStream]");
	// send to agent (end msg)
	App()->SendStreamStatusToAgent(false);
	RequestCloseRoom();
}

void MainWindow::OnCloseRoom(bool success)
{
	elogi("[MainWindow::OnCloseRoom]");
	if (m_closing && LimeCommunicator::Instance()->GetStreamingState() == eStreamingState::eSS_Stopping)
	{
		m_closeRoomComplete = true;
		if (!m_closingMessage)
			QApplication::quit();
	}
}

void MainWindow::OnError()
{
	elogi("[MainWindow::OnError]");
	if (m_closing && LimeCommunicator::Instance()->GetStreamingState() == eStreamingState::eSS_Stopping)
	{
		m_closeRoomComplete = true;
		if (!m_closingMessage)
			QApplication::quit();
	}
}

void MainWindow::OnEventCount(StreamRoomInfo& streamRoomInfo)
{
	elogd("[MainWindow::OnEventCount]");
	setViewerCount(streamRoomInfo.watchingCount);
	ResetGeoViewerPopUp();
}

void MainWindow::OnEventChatGroupLeave()
{
	elogd("[MainWindow::OnEventCount]");
	AppUtils::DispatchToMainThread([] {
		MainWindow::Get()->AppClose(AppString::GetString("event_message/event_leave_chat_group").c_str());
	});
}

void MainWindow::OnEventJoin(StreamRoomUserInfo& streamRoomUserInfo, StreamRoomUpdateStatusInfo& streamRoomUpdateStatusInfo)
{
	elogd("[MainWindow::OnEventJoin]");
	m_viewerlistpopup->AddViewer(streamRoomUserInfo);

	setViewerCount(m_viewerlistpopup->getViewerCount());
	ResetGeoViewerPopUp();
}

void MainWindow::OnEventLeave(StreamRoomUserInfo& streamRoomUserInfo, StreamRoomUpdateStatusInfo& streamRoomUpdateStatusInfo)
{
	elogd("[MainWindow::OnEventLeave]");
	m_viewerlistpopup->RemoveViewer(streamRoomUserInfo.gameUserId);
	setViewerCount(m_viewerlistpopup->getViewerCount());
	ResetGeoViewerPopUp();
}

void MainWindow::OnEventDeport(StreamRoomUserInfo& streamRoomUserInfo, StreamRoomUpdateStatusInfo& streamRoomUpdateStatusInfo)
{
	elogd("[MainWindow::OnEventDeport]");
	m_viewerlistpopup->RemoveViewer(streamRoomUserInfo.gameUserId);
	setViewerCount(m_viewerlistpopup->getViewerCount());
	ResetGeoViewerPopUp();

	std::string message = AppUtils::format(AppString::GetString("MainDlg/deport_viewer_message").c_str(), streamRoomUserInfo.name.c_str());
	VideoChatMessageBox::Instance()->information(message.c_str()); 
}

void MainWindow::OnEventStopStream(bool isForceQuit)
{
	LOG(INFO) << format_string("[MainWindow::OnEventStopStream] isForceQuit(%d)", isForceQuit);

	StopStreaming();
}

void MainWindow::OnMediaSeverError(int error, const char* msg)
{
	LOG(INFO) << format_string("[MainWindow::OnMediaSeverError] error(%d)(%s)", error, msg);

	switch (error)
	{
		case eErrorCode::PC_ConnectionDisconnected:
			break;
		case eErrorCode::PC_ConnectionClosed:
		case eErrorCode::PC_ConnectionFailed:
		default:
			{
			AppUtils::DispatchToMainThread([error] {
				MainWindow::Get()->AppClose(AppUtils::format(AppString::GetString("error_message/error_media_server").c_str(), error).c_str());
			});
			}
			break;
	}
}

void MainWindow::OnSignalServerError(int error, const char* msg)
{
	LOG(INFO) << format_string("[MainWindow::OnSignalServerError] error(%d)(%s)", error, msg);

	AppUtils::DispatchToMainThread([error] {
		MainWindow::Get()->AppClose(AppUtils::format(AppString::GetString("error_message/error_signal_server").c_str(), error).c_str());
	});	
}

void MainWindow::CreatePopUp()
{
	m_viewerlistpopup = new ViewerListPopUp(this);
	m_viewerlistpopup->setPopUpSize(m_viewerlistpopup->getViewPopUpWidth(), m_viewerlistpopup->getViewPopUpHeight());

	m_viewerlistpopup->setFocus();
	m_viewerlistpopup->installEventFilter(this);
}

void MainWindow::ResetGeoViewerPopUp()
{
	if (m_viewerlistpopup->getViewerCount() == 0 && m_viewerlistpopup->isVisible()) m_viewerlistpopup->hide();

	QPoint pt = ui.viewer_list_icon_btn->pos();
	int titlebar_y = ui.widget_titlebar->y() + ui.widget_titlebar->height();
	int setting_y = ui.widget_setting->y();
	QPoint p = mapToGlobal(QPoint(pt.x() - 8, pt.y() - 8 + setting_y - titlebar_y - m_viewerlistpopup->getViewPopUpHeight() + 40/*verticalspace height*/ - 5));

	QPoint pp = getPopUpGeometry(p.x(), p.y(), m_viewerlistpopup->getViewPopUpWidth(), m_viewerlistpopup->getViewPopUpHeight() + 1);
	QRect newr(QPoint(pp.x(), pp.y() - m_viewerlistpopup->GetYMargin()), QSize(m_viewerlistpopup->getViewPopUpWidth(), m_viewerlistpopup->getViewPopUpHeight()));
	m_viewerlistpopup->setPopUpSize(newr.width(), newr.height());

	m_viewerlistpopup->setGeometry(newr);
}

void MainWindow::on_viewer_list_icon_btn_clicked()
{
	if (m_viewerlistpopup->isVisible()) {
		m_viewerlistpopup->hide();
	}
	else {
		if (m_viewerlistpopup->getViewerCount() == 0) return;
		ResetGeoViewerPopUp();
		m_viewerlistpopup->show();
	}
}

void MainWindow::on_pushButton_share_clicked()
{
	QApplication::clipboard()->setText(LimeCommunicator::Instance()->GetInviteLinkUrl().c_str(), QClipboard::Clipboard);
	VideoChatMessageBox::Instance()->information(AppString::GetString("MainDlg/url_share_main_message"), AppString::GetString("MainDlg/url_share_sub_message"));
}

void MainWindow::on_pushButton_setting_clicked()
{
	if (m_viewerlistpopup->isHidden() == false) m_viewerlistpopup->hide();

	constexpr int padding = 11;
	constexpr int margin_right = 90 + 8;
	constexpr int margin_bottom = 72;
	constexpr int menu_width = 176 + padding;
	constexpr int menu_height = 116 + padding; 
	
	int x = this->pos().x() + size().width() - (margin_right + menu_width);
	int y = this->pos().y() + size().height() - (margin_bottom+menu_height);

	MenuControl::Instance()->ShowMainMenu(this, {x,y});
}

QPoint MainWindow::getPopUpGeometry(int _x, int _y, int _width, int _heigth)
{
	QPoint qRreturn;
	int nx = _x;
	int ny = _y;
	QScreen *screen = AppUtils::GetActiveScreen(this);
	QRect rec = screen->geometry();
	int height = rec.height();
	int width = rec.width();

	if (rec.x() > _x) {
		nx = rec.x();
	}
	if (rec.x() + width < _x + _width) {
		nx = rec.x() + width - _width;
	}

	if (rec.y() > _y) {
		ny = rec.y();
	}
	if (rec.y() + height < _y + _heigth) {
		ny = rec.y() + height - _heigth;
	}

	qRreturn.setX(nx);
	qRreturn.setY(ny);

	return qRreturn;
}

void MainWindow::on_close_btn_clicked()
{
	on_pushButton_quit_clicked();
}

void MainWindow::on_min_btn_clicked()
{
	ShowWindow(HWND(winId()), SW_MINIMIZE);
}

void MainWindow::on_max_btn_clicked()
{
	if (isMaximized())
	{
		setWindowState(Qt::WindowNoState);
	}
	else
	{
		setWindowState(Qt::WindowMaximized);
	}
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
	CheckHideInfoBox(event);
}

void MainWindow::CheckHideInfoBox(QResizeEvent* event)
{
	int labelWidth = ui.stream_title_label->width();
	int buttonWidth = ui.pushButton_quit->x() - ui.viewer_list_icon_btn->x() + ui.pushButton_quit->width();

	int _width = 0;
	if (event == nullptr)
	{
		_width = width();
	}
	else
	{
		_width = event->size().width();
	}

	if (_width > labelWidth + buttonWidth + HIDE_WIDTH_X_MARGIN)
	{
		ui.spacer_widget->hide();
		ui.hide_widget->show();
	}
	else
	{
		ui.hide_widget->hide();
		ui.spacer_widget->show();
	}
}

void MainWindow::closeEvent(QCloseEvent * event)
{
	if (isMinimized()) setWindowState(Qt::WindowNoState);

	if (VideoChatMessageBox::Instance()->question(AppString::GetString("MainDlg/quit_question")))
	{
		AppClose();
	}
	else {
		event->ignore();
	}
}

void MainWindow::on_pushButton_quit_clicked()
{
	if (VideoChatMessageBox::Instance()->question(AppString::GetString("MainDlg/quit_question")))
	{
		AppClose();
	}
	else {
		// 무시
	}
}

QPoint MainWindow::GetViewerListIconPos()
{
	return ui.viewer_list_icon_btn->pos();
}

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
#if (QT_VERSION == QT_VERSION_CHECK(5, 11, 1))
	MSG* msg = *reinterpret_cast<MSG**>(message);
#else
	MSG* msg = reinterpret_cast<MSG*>(message);
#endif

	switch (msg->message)
	{
	case WM_MOVE:
	case WM_SIZING:
		if (m_viewerlistpopup) {
			if (m_viewerlistpopup->isVisible()) {
				QPoint pt = ui.viewer_list_icon_btn->pos();

				int titlebar_y = ui.widget_titlebar->y() + ui.widget_titlebar->height();
				int setting_y = ui.widget_setting->y();
				int _y = pt.y() + setting_y - titlebar_y - m_viewerlistpopup->getViewPopUpHeight() + 40/*verticalspace height*/ - 5;
				QPoint p = mapToGlobal(QPoint(pt.x() - 8, _y - 8));

				QPoint pp = getPopUpGeometry(p.x(), p.y(), m_viewerlistpopup->getViewPopUpWidth(), m_viewerlistpopup->getViewPopUpHeight());
				QRect newr(QPoint(pp.x(), pp.y() - m_viewerlistpopup->GetYMargin()), QSize(m_viewerlistpopup->getViewPopUpWidth(), m_viewerlistpopup->getViewPopUpHeight()));
				m_viewerlistpopup->setPopUpSize(newr.width(), newr.height());
				m_viewerlistpopup->setGeometry(newr);
			}
		}

		m_changed_size_or_pos = true;
		emit ResetChildGeometry();
		break;
	}
	return QFramelessDialog::nativeEvent(eventType, message, result);
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{

}

void MainWindow::PreviewResize()
{
	return ui.widget_preview_container->Resize();
}

PreviewWidget* MainWindow::GetPreviewWidget()
{
	return ui.widget_preview;
}

bool MainWindow::event(QEvent *event)
{
	int clicked_x = QWidget::mapFromGlobal(QCursor::pos()).x();
	int clicked_y = QWidget::mapFromGlobal(QCursor::pos()).y();
	switch (event->type())
	{
	case QEvent::MouseButtonPress:
		if (getBorderSize() < clicked_x &&
			getBorderSize() < clicked_y &&
			this->width() - getBorderSize() > clicked_x &&
			this->height() - getBorderSize() > clicked_y) 
		{
			if (m_viewerlistpopup->isVisible()) m_viewerlistpopup->hide();
		}
		break;
	case QEvent::MouseButtonRelease:
		{
			if (m_changed_size_or_pos)
			{
				m_changed_size_or_pos = false;
				LocalSettings::SetWindowGeometry(metaObject()->className(), geometry());
			}
		}
		break;
	case QEvent::WindowDeactivate:
		mb_mainWin_Hasfocus = false;
		if (!mb_mainWin_Hasfocus && !mb_viewerPopup_Hasfocus) m_viewerlistpopup->hide();
		break;
	case QEvent::WindowActivate:
		mb_mainWin_Hasfocus = true;
		emit OnActiveWindow();
		break;
	case QEvent::WindowStateChange:
		LocalSettings::SetWindowMaximized(metaObject()->className(), isMaximized());
		ui.max_btn->setProperty("PrintMaxIcon", isMaximized());
		ui.max_btn->style()->unpolish(ui.max_btn);
		ui.max_btn->style()->polish(ui.max_btn);
		break;
	}

	return QFramelessDialog::event(event);
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event)
{
	if (obj == ui.time_label) {
		if (event->type() == QEvent::Paint) {
			QPainter painter;

			painter.begin(ui.time_label);

			if(m_p_streamTimer) painter.setPen(m_TimerLabelColor);
			else painter.setPen(m_TimerLabelDisableColor);

			painter.setFont(m_streamTimerFont);
	
			int __y = ui.time_label->height() / 2 - 20 / 2 - 1;
			int i = 0;

			QString strHour = QString::number(m_stream_h);

			if (strHour.length() == 1) {
				painter.drawText(STREAM_TIMER_LETTER_WIDTH * i++, __y, STREAM_TIMER_WORD_WIDTH, STREAM_TIMER_WORD_HEIGHT, Qt::AlignLeft, "0");
				painter.drawText(STREAM_TIMER_LETTER_WIDTH * i++ + 1, __y, STREAM_TIMER_WORD_WIDTH, STREAM_TIMER_WORD_HEIGHT, Qt::AlignLeft, strHour.at(i-1));
			}
			else {
				painter.drawText(STREAM_TIMER_LETTER_WIDTH * i++, __y, STREAM_TIMER_WORD_WIDTH, STREAM_TIMER_WORD_HEIGHT, Qt::AlignLeft, strHour.at(i));
				for (; i < strHour.length(); i++) {
					painter.drawText(STREAM_TIMER_LETTER_WIDTH * i + 1, __y, STREAM_TIMER_WORD_WIDTH, STREAM_TIMER_WORD_HEIGHT, Qt::AlignLeft, strHour.at(i));
				}
			}

			painter.drawText(STREAM_TIMER_LETTER_WIDTH * i++ + STREAM_TIMER_X_MARGIN_2, __y, STREAM_TIMER_WORD_WIDTH, STREAM_TIMER_WORD_HEIGHT, Qt::AlignLeft, ":");

			painter.drawText(STREAM_TIMER_LETTER_WIDTH * i++, __y, STREAM_TIMER_WORD_WIDTH, STREAM_TIMER_WORD_HEIGHT, Qt::AlignLeft, QString::number(m_stream_m / 10));
			painter.drawText(STREAM_TIMER_LETTER_WIDTH * i++ + 1, __y, STREAM_TIMER_WORD_WIDTH, STREAM_TIMER_WORD_HEIGHT, Qt::AlignLeft, QString::number(m_stream_m % 10));

			painter.drawText(STREAM_TIMER_LETTER_WIDTH * i++ + STREAM_TIMER_X_MARGIN_2, __y, STREAM_TIMER_WORD_WIDTH, STREAM_TIMER_WORD_HEIGHT, Qt::AlignLeft, ":");

			painter.drawText(STREAM_TIMER_LETTER_WIDTH * i++, __y, STREAM_TIMER_WORD_WIDTH, STREAM_TIMER_WORD_HEIGHT, Qt::AlignLeft, QString::number(m_stream_s / 10));
			painter.drawText(STREAM_TIMER_LETTER_WIDTH * i + 1, __y, STREAM_TIMER_WORD_WIDTH, STREAM_TIMER_WORD_HEIGHT, Qt::AlignLeft, QString::number(m_stream_s % 10));

			painter.end();

			return true;
		}
	}

	if (obj == ui.min_btn || obj == ui.close_btn)
	{
		if (event->type() == QEvent::MouseButtonPress) {
			QPoint widgetCursorPos = QWidget::mapFromGlobal(QCursor::pos());

			int x = widgetCursorPos.x();
			int y = widgetCursorPos.y();
			QSize size = this->size();

			int borderWidth = getBorderSize();

			ReleaseCapture();
			if (x > size.width() - borderWidth && y < borderWidth) {
				this->setCursor(QCursor(Qt::SizeBDiagCursor));
				SendMessage(HWND(winId()), WM_NCLBUTTONDOWN, HTTOPRIGHT, 0);
				return true;
			}
			else if (y < borderWidth) {
				this->setCursor(QCursor(Qt::SizeVerCursor));
				SendMessage(HWND(winId()), WM_NCLBUTTONDOWN, HTTOP, 0);
				return true;
			}
			else if (x > size.width() - borderWidth) {
				this->setCursor(QCursor(Qt::SizeHorCursor));
				SendMessage(HWND(winId()), WM_NCLBUTTONDOWN, HTRIGHT, 0);
				return true;
			}
			return false;
		}
		else {
			return QFramelessDialog::eventFilter(obj, event);
		}
	}
	else if (obj == m_viewerlistpopup)
	{
		if (event->type() == QEvent::WindowDeactivate) {
			mb_viewerPopup_Hasfocus = false;
			if (!mb_mainWin_Hasfocus && !mb_viewerPopup_Hasfocus) m_viewerlistpopup->hide();
		}
		else if (event->type() == QEvent::WindowActivate) {
			mb_viewerPopup_Hasfocus = true;
		}
	}
	else if (obj == ui.roomName_label)
	{
		if (event->type() == QEvent::Paint)
		{
			QPainter painter(ui.roomName_label);

			QString characterName = m_title;
			QString characterSuffix = AppString::GetQString("StreamDlg/default_room_title_suffix");
			QString characterPreffix = AppString::GetQString("StreamDlg/default_room_title_prefix");

			int _x = 0;
			int _y = ui.roomName_label->height() / 2 - 20 / 2;
			int nameWidth = DrawUtils::GetFontWidth(AppUtils::GetBoldFont(TITLEBAR_FONT_SIZE), characterName);
			int prefixWidth = DrawUtils::GetFontWidth(AppUtils::GetBoldFont(TITLEBAR_FONT_SIZE), characterPreffix);
			int suffixWidth = DrawUtils::GetFontWidth(AppUtils::GetBoldFont(TITLEBAR_FONT_SIZE), characterSuffix);

			// Draw Text - prefix
			painter.setPen(m_titlebarSuffix);
			painter.setFont(AppUtils::GetBoldFont(TITLEBAR_FONT_SIZE));
			painter.drawText(
				_x,												// x
				_y,												// y
				prefixWidth,									// width
				20,												// height
				Qt::AlignLeft, characterPreffix);

			// Draw Text - character name
			painter.setPen(m_titlebarCharacterName);
			painter.setFont(AppUtils::GetBoldFont(TITLEBAR_FONT_SIZE));
			painter.drawText(
				_x + prefixWidth,								// x
				_y,												// y
				nameWidth,										// width
				20,												// height
				Qt::AlignLeft, characterName);

			// Draw Text - suffix
			painter.setPen(m_titlebarSuffix);
			painter.drawText(
				_x + prefixWidth + nameWidth,					// x
				_y,												// y
				suffixWidth,									// width
				20,												// height
				Qt::AlignLeft, characterSuffix);

			painter.end();
		}
	}

	return QFramelessDialog::eventFilter(obj, event);
}

void MainWindow::SetTitle(QString title)
{
	m_title = title;
	title = AppString::GetQString("StreamDlg/default_room_title_prefix") + title + AppString::GetQString("StreamDlg/default_room_title_suffix");
	ui.stream_title_label->setText(title);
	ui.roomName_label->setFixedWidth(DrawUtils::GetFontWidth(AppUtils::GetBoldFont(TITLEBAR_FONT_SIZE), title) + 10);
	ui.stream_title_label->setFixedWidth(DrawUtils::GetFontWidth(AppUtils::GetBoldFont(INFORMBOX_FONT_SIZE), title));
	setWindowTitle(title);
	ui.widget_titlebar->update();
}

QString MainWindow::GetTitle()
{
	return windowTitle();
}

void MainWindow::UpdateLanguageString()
{
	ui.pushButton_quit->setText(AppString::GetQString("Common/close"));
	SetTitle(m_title);

	emit UpdateChildLanguageString();
	update();
}