#pragma once

#include <qdialog.h>
#include <QMainWindow>

#define DEFAULT_BORDER_SIZE 5

class QFramelessDialog : public QMainWindow
{
public:
	explicit QFramelessDialog(QWidget *parent = nullptr, 
								bool _isFrameless = true, 
								uint8_t _borderwidth = DEFAULT_BORDER_SIZE,
								bool _useResize = true,
								float _aspectratio = 0.0f);
	~QFramelessDialog() {}

	void SetEnableMaximize(bool enable) { m_enableMaximize = enable; }

protected:
	void SetEraseBackGround(QColor color);
	bool nativeEvent(const QByteArray &eventType, void *message, long *result);
	bool event(QEvent *event);
	const uint8_t getBorderSize() { return m_borderWidth; }

private:
	virtual const int getTitlebarHeight() = 0;
	virtual const int getTitlebarWidth() = 0;

	bool m_isClicked{false};
	bool m_eraseBackGround{false};
	bool m_enableMaximize{true};
	int m_doubleClickInterval{100};
	float m_aspectRatio;
	QColor m_backGroundColor{};
	const uint8_t m_borderWidth;
	const bool m_enableResize;
};