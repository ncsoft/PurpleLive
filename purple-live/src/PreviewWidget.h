#pragma once

#include <QWidget>
#include <qelapsedtimer.h>
#include <qvariantanimation.h>
#include <NCCaptureSDK.h>
#include <qtimer.h>

class PreviewWidget : public QWidget
{
	Q_OBJECT

public:
	PreviewWidget(QWidget *parent = Q_NULLPTR);
	~PreviewWidget();

	virtual QPaintEngine *paintEngine() const override;
	void	Init();
	void	InitMask(const char* game_code);
	void	TryRefreshSprite();
	void	StartRender(const char* game_code);
	void	StopRender();
	QSize	GetSourceSize();

public slots:
	void animate();

protected:
	void	resizeEvent(QResizeEvent* event) override;
	void	paintEvent(QPaintEvent* event) override;

private:
	int	m_elapsed;
	GameCaptureSprite*	m_pSprite = nullptr;
	QTimer	m_timer;

	bool	m_paint_on_screen = false;
	int		m_src_width;
	int		m_src_height;
	int		m_hook_ready_key;

	int		m_dst_width;
	int		m_dst_height;

	void	_resize();
	void	_timerProc();
};
