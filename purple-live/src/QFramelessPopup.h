#pragma once

#include <qwidget.h>

class QFramelessPopup : public QWidget
{
public:
	explicit QFramelessPopup(QWidget *parent = nullptr, bool _isFrameless = true);
	~QFramelessPopup() {}

protected:
	void SetEraseBackGround(QColor color);
	bool nativeEvent(const QByteArray &eventType, void *message, long *result);
	bool event(QEvent *event);

private:
	virtual const int getTitlebarHeight() abstract;
	virtual const int getTitlebarWidth() abstract;

	bool m_eraseBackGround = false;
	QColor m_backGroundColor;
};