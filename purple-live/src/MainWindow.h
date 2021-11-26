#pragma once

#include <QtWidgets/QMainWindow>
#include <QPointer>

#include "ui_MainWindow.h"

#include "StartUpWindow.h"
#include "StartUpLauncherWindow.h"
#include "QFramelessDialog.h"
#include "lime-defines.h"
#include "LimeCommunicator.h"
#include "ViewerListPopUp.h"
#include "AudioMixer/NCAudioMixer.h"

#define	STREAM_TIMER_X_MARGIN_2					3
#define STREAM_TIMER_LETTER_WIDTH				7
#define STREAM_TIMER_WORD_WIDTH					58
#define STREAM_TIMER_WORD_HEIGHT				20
#define TITLEBAR_ICON_WIDTH						21
#define TITLEBAR_ICON_HEIGHT					16
#define TITLEBAR_FONT_SIZE						13
#define	HIDE_WIDTH_X_MARGIN						80
#define INFORMBOX_FONT_SIZE						16

class ViewerListPopUp;

class MainWindow : public QFramelessDialog, public LimeEventHandler
{
    Q_OBJECT
	Q_PROPERTY(QColor backGround WRITE SetBackGroundColor READ GetBackGroundColor)
	Q_PROPERTY(QColor timerLabel WRITE SetTimerLabel READ GetTimerLabel)
	Q_PROPERTY(QColor timerLabelDisable WRITE SetTimerLabelDisable READ GetTimerLabelDisable)
	Q_PROPERTY(QColor titlebarCharacterName WRITE SetTitlebarCharacterName READ GetTitlebarCharacterName)
	Q_PROPERTY(QColor titlebarSuffix WRITE SetTitlebarSuffix READ GetTitlebarSuffix)
		
public:
    MainWindow(QWidget *parent = Q_NULLPTR);
	~MainWindow();

	static MainWindow*	Get();
	static QWidget* GetActiveWidget();

	bool	GetActive();
	void	SetActive(bool enable);

	bool	Init();
	void	SetStreamingSize(int width, int height);
	void	SetTarget(HWND hwnd);
	void	StopRender();
	bool	StartRender(bool isForceExcute, const char* strRoomID, const char* strRoomName=nullptr, const char* strGroupID=nullptr, const char* strChannelID=nullptr);
	void	SetChangeStreamingStateCallback(std::function<void (eStreamingState streamingState)>);
	void	BringToFront();
	void	RetryExcution();

	QString GetMicNameFromIndex(int index);
	void	GetMicInfoList(std::vector<AudioDeviceInfo>& mic_list);
	void	SetMicSelect(int index);
	void	SetAudioOutput();
	int		GetMicIndex() { return m_currentMicIndex; }
	void	SetMicIndex(int index) { m_currentMicIndex = index; }
	float	GetMicVolume();
	void	SetMicVolume(float volume);
	void	SetEnableInputAudioCapture(bool enable);
	void	SetLocalSavedMicSetting();

	void	SetBossHide(bool enable);
	void	SetScreenLock(bool enable);
	void	AppClose(const char* message=nullptr);
	void	OnLanguageChanged();

	void	CreateStartUpGameWindow();
	void	CreateStartUpLauncherWindow();

	bool	SetActiveStartUpGameWindow(bool is_active);
	void	SetActiveStartUpLauncherWindow(bool is_active);

	void	RequestStartStreaming();
	void	RequestStopStreaming();
	void	RequestCloseRoom();
	
	void	RequestCreateChatting(const GuildUserInfoList& info);
	bool	RequestGuildUserList();
	void	RequestShareTargetList();
	void	RequestRoomUserList(bool isAppend=false, int count=100);
	void	RequestWatchingUserList(bool isAppend=false, int count=100);
	void	RequestDeportRoom(const DeportUserInfo& info);

	void	StartStreaming(MediaSignalServerInfo& mediaSignalServerInfo, const char* strAuthKey);
	void	StopStreaming();

	void	SetMuted(bool muted, bool disable = false);

	// Lime Event Handlder
	void	OnPubServerConnected(bool reconnect) override;
	void	OnChangeStreamingState(eStreamingState streamingState) override;
	void	OnCreateChatting(ChattingInfo& info, bool success) override;
	void	OnGetGuildUserList(GuildUserInfoList& info, bool success) override;
	void	OnGetShareTargetList(vector<GroupInfo>& groupInfoList, bool success) override;
	void	OnGetRoomUserList(StreamRoomUserInfoList& viewerList, bool success) override;
	void	OnGetWatchingUserList(StreamRoomUserInfoList& viewerList, bool success) override;
	void	OnDeportRoom(bool success) override;
	void	OnReconnectRoom(StreamRoomInfo& roomInfo, StreamRoomUserInfo& ownerInfo, bool success) override;
	void	OnCreateRoom(StreamRoomInfo& roomInfo, StreamRoomUserInfo& ownerInfo, bool success) override;
	void	OnJoinRoom(bool success) override;
	void	OnUpdateRoom(StreamRoomInfo& streamRoomInfo, bool success) override;
	void	OnStartStream(MediaSignalServerInfo& mediaSignalServerInfo, const char* strAuthKey, bool success) override;
	void	OnStopStream(bool success) override;
	void	OnCloseRoom(bool success) override;
	void	OnError() override;
	void	OnEventCount(StreamRoomInfo& streamRoomInfo) override;
	void	OnEventChatGroupLeave() override;
	void	OnEventJoin(StreamRoomUserInfo& streamRoomUserInfo, StreamRoomUpdateStatusInfo& streamRoomUpdateStatusInfo) override;
	void	OnEventLeave(StreamRoomUserInfo& streamRoomUserInfo, StreamRoomUpdateStatusInfo& streamRoomUpdateStatusInfo) override;
	void	OnEventDeport(StreamRoomUserInfo& streamRoomUserInfo, StreamRoomUpdateStatusInfo& streamRoomUpdateStatusInfo) override;
	void	OnEventStopStream(bool isForceQuit) override;

	// MediaRoom Event Handler
	void	OnMediaSeverError(int error, const char* msg);
	void	OnSignalServerError(int error, const char* msg);

	bool getStartUpWindowIsHidden() { return m_startUpWindow->isHidden(); }
	void setViewerCount(const int _viewernum);
	void ResetGeoViewerPopUp();
	void PreviewResize();

	StartUpWindow* GetStartupWindow() { return m_startUpWindow; }
	PreviewWidget* GetPreviewWidget();
	
	QPoint GetViewerListIconPos();
	QString GetTitle();

	void SetTitle(QString title);
	void SetStartStreamUI();
	void SetStopStreamUI();
	void SetMicInputLevel(float micInput);

public slots: 
	void UpdateLanguageString();

#ifdef _DEBUG
protected:
	void keyPressEvent(QKeyEvent* event);
#endif//_DEBUG

private:
    Ui::MainWindowClass ui;

	bool m_active = false;
	bool m_closing = false;
	bool m_closingMessage = false;
	bool m_closeRoomComplete = false;
	bool m_changed_size_or_pos = false;
	bool mb_settingPopup_Hasfocus = true;
	bool mb_viewerPopup_Hasfocus = true;
	bool mb_mainWin_Hasfocus = true;
	QFont m_streamTimerFont;
	int m_stream_h = 0, m_stream_m = 0, m_stream_s = 0;		// timer value
	int m_currHourDigits = 2;
	int m_currentMicIndex = -1;

	QString m_title;
	HWND m_targetHwnd = nullptr;
	std::vector<AudioDeviceInfo> m_vecMicInfoList;

	QPointer<QTimer>					m_p_streamTimer;
	QPointer<StartUpWindow>				m_startUpWindow;
	QPointer<StartUpLauncherWindow>		m_startUpLauncherWindow;
	QPointer<ViewerListPopUp>			m_viewerlistpopup;

	std::function<void (eStreamingState streamingState)> m_streamStateCallback;

	void InitUI();
	void InitUIAfterCreation();
	void CreatePopUp();
	QPoint getPopUpGeometry(int _x, int _y, int _width, int _heigth);
	virtual const int getTitlebarHeight() { return ui.widget_titlebar->height(); }
	virtual const int getTitlebarWidth() { return ui.widget_titlebar->width(); }
	bool nativeEvent(const QByteArray &eventType, void *message, long *result);
	void contextMenuEvent(QContextMenuEvent *event) override;
	
signals:
	void StyleChanged();
	void UpdateChildLanguageString();
	void OnActiveWindow();
	void ResetChildGeometry();

private slots:
	bool event(QEvent *event);
	void resizeEvent(QResizeEvent* event);
	void CheckHideInfoBox(QResizeEvent* event = nullptr);
	bool eventFilter(QObject* obj, QEvent* event);
	void onStreamTimer();
	void on_pushButton_share_clicked();
	void on_pushButton_setting_clicked();
	void on_viewer_list_icon_btn_clicked();
	void on_close_btn_clicked();
	void on_min_btn_clicked();
	void on_max_btn_clicked();
	void on_pushButton_quit_clicked();
	void on_mute_pushButton_clicked();
	void closeEvent(QCloseEvent * event);

private:
	// q-property value, func
	QColor m_backGroundColor;
	QColor m_TimerLabelColor;
	QColor m_TimerLabelDisableColor;
	QColor m_titlebarCharacterName;
	QColor m_titlebarSuffix;

	void SetBackGroundColor(QColor color) { m_backGroundColor = color; }
	void SetTimerLabel(QColor color) { m_TimerLabelColor = color; }
	void SetTimerLabelDisable(QColor color) { m_TimerLabelDisableColor = color; }
	void SetTitlebarCharacterName(QColor color) { m_titlebarCharacterName = color; }
	void SetTitlebarSuffix(QColor color) { m_titlebarSuffix = color; }

	QColor GetBackGroundColor() { return m_backGroundColor; }
	QColor GetTimerLabel() { return m_TimerLabelColor; }
	QColor GetTimerLabelDisable() { return m_TimerLabelDisableColor; }
	QColor GetTitlebarCharacterName() { return m_titlebarCharacterName; }
	QColor GetTitlebarSuffix() { return m_titlebarSuffix; }
};
