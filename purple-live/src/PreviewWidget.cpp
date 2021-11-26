#include "PreviewWidget.h"
#include "CommunityLiveManager.h"
#include "easylogging++.h"
#include <NCCaptureSDK.h>
#include <qtimer.h>
#include <chrono>
#include "LocalSettings.h"
#include "MaskingManager.h"
#include "AppConfig.h"
#include "AppUtils.h"
#include "MainWindow.h"
#include "VideoChatApp.h"


PreviewWidget::PreviewWidget(QWidget *parent)
	: QWidget(parent)
{
	LOG(DEBUG) << format_string("[PreviewWidget::PreviewWidget]");

	m_elapsed = 0;

	QWidget::setAttribute(Qt::WA_OpaquePaintEvent);
	QWidget::setAttribute(Qt::WA_StaticContents);
	QWidget::setAttribute(Qt::WA_NoSystemBackground);
	QWidget::setAttribute(Qt::WA_NativeWindow);
}

PreviewWidget::~PreviewWidget()
{
	LOG(DEBUG) << format_string("[PreviewWidget::~PreviewWidget]");

	StopRender();

	if (m_pSprite)
	{
		delete m_pSprite;
	}
}

QPaintEngine * PreviewWidget::paintEngine() const
{
	return nullptr;
}

void OnVideoDeviceCreated(void* user_data, bool recreate)
{
	LOG(INFO) << format_string("[PreviewWidget::OnVideoDeviceCreated] recreate(%d)", recreate);

	if (!recreate)
		return;

	// recreate masking sprite
	MaskingManager::Instance()->Setup(App()->GetOriginGameCode());
}

void PreviewWidget::Init()
{
	LOG(DEBUG) << format_string("[PreviewWidget::Init]");

	m_dst_width = width();
	m_dst_height = height();

	NCCapture::Instance()->InitLog(AppUtils::GetLogPath().c_str());
	NCCapture::Instance()->SetEnableLog(AppConfig::enable_log_error, AppConfig::enable_log_info, AppConfig::enable_log_debug);
	NCCapture::Instance()->RegisterDeviceCreatedCallback(OnVideoDeviceCreated, this);
	NCCapture::Instance()->CreateVideoDevice((HWND)winId(), AppConfig::Instance()->streaming_width, AppConfig::Instance()->streaming_height);
}

void PreviewWidget::InitMask(const char* game_code)
{
	std::vector<std::wstring> mask_list;
	MaskingManager::Instance()->GetMaskNameList(mask_list);
	bool is_enable_game_mask = LocalSettings::GetEnableGameMask(LocalSettings::DEFAULT_ENABLE_GAME_MASK);
	QBitArray game_mask_settings = LocalSettings::GetGameMaskSettings(game_code, QBitArray(mask_list.size()));
	MaskingManager::Instance()->GetMaskNameList(mask_list);
	for (int i=0; i<mask_list.size(); i++)
	{
		MaskingManager::Instance()->SetVisible(i, (is_enable_game_mask && game_mask_settings[i]));
	}
}

void PreviewWidget::TryRefreshSprite()
{
	int width, height;
	bool flip;
	int hook_ready_key;
	ID3D11ShaderResourceView* pSRV = (ID3D11ShaderResourceView*)NCCapture::Instance()->GetCapturedSRV(width, height, flip, hook_ready_key);
	if (pSRV==nullptr)
		return;
	
	// sprite 재생성 조건 : 게임 해상도 변경, 캡처 초기화(graphics api reset)
	if (m_pSprite==nullptr || width != m_src_width || height != m_src_height || hook_ready_key != m_hook_ready_key)
	{
		m_src_width = width;
		m_src_height = height;
		m_hook_ready_key = hook_ready_key;
		delete m_pSprite;

		LOG(INFO) << format_string("[PreviewWidget::TryRefreshSprite] source size(%d %d) hook_ready_key(%d)", m_src_width, m_src_height, m_hook_ready_key);

		MainWindow::Get()->PreviewResize();
		m_pSprite = new GameCaptureSprite();
		m_pSprite->Create(pSRV, flip);
	}
}

void PreviewWidget::StartRender(const char* game_code)
{
	LOG(DEBUG) << format_string("[PreviewWidget::StartRender]");

	InitMask(game_code);

	m_timer.setInterval(1000/AppConfig::Instance()->streaming_framerate);
	m_timer.setTimerType(Qt::PreciseTimer);
	QObject::connect(&m_timer, &QTimer::timeout, this, &PreviewWidget::_timerProc, Qt::DirectConnection);
	m_timer.start();
}

void PreviewWidget::StopRender()
{
	m_timer.stop();
}

QSize PreviewWidget::GetSourceSize()
{
	return QSize(m_src_width, m_src_height);
}

void PreviewWidget::animate()
{
	m_elapsed = (m_elapsed + qobject_cast<QTimer*>(sender())->interval()) % 1000;
	update();
}

void PreviewWidget::resizeEvent(QResizeEvent * event)
{
	_resize();
}

void PreviewWidget::paintEvent(QPaintEvent* event)
{
	if (m_paint_on_screen)
		return;

	QPainter painter(this);
	painter.fillRect(rect(), QBrush(Qt::black));	
	painter.end();
	QWidget::paintEvent(event);
}

void PreviewWidget::_resize()
{
	int width = this->width();
	int height = this->height();
	NCCapture::Instance()->OnResizing(width, height);
}

void PreviewWidget::_timerProc()
{
	if (!NCCapture::Instance()->IsCapturing())
		return;

	if (!m_paint_on_screen)
	{
		QWidget::setAttribute(Qt::WA_PaintOnScreen);
		m_paint_on_screen = true;
	}

	NCCapture::Instance()->Update();
	
	// 게임 해상도 변경, 캡처 초기화(graphics api reset) 체크
	TryRefreshSprite();
	
	// render
	NCCapture::Instance()->BeginRender();
	if (m_pSprite)
	{
		m_pSprite->DrawFull();
	}

	MaskingManager::Instance()->RenderMasks();
	NCCapture::Instance()->EndRender();

	// streaming
	if (m_pSprite)
	{
		const CapturedImage* pImage = NCCapture::Instance()->GetFrameBufferImage();
		video_data* vdata = CommunityLiveManager::Instance()->GetVideoData();
		vdata->data[0] = pImage->pixels;
		CommunityLiveManager::Instance()->OnVideoFrame(vdata, pImage->width, pImage->height, true);			
	}
}
