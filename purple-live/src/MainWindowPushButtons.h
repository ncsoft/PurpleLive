#pragma once

#include <qpushbutton.h>

#define DEFAULT_ICON_SIZE				24
#define SETTING_ICON_SIZE				24
#define VIEWER_BTN_IMAGE_X_MARGIN		18
#define VIEWER_BTN_FONT_SIZE			14
#define VIEWER_BTN_EXTRA_WIDTH			49+15
#define	VIEWER_MAXIMUM_NUM				9999

class MainWindowSettingButton : public QPushButton
{
	Q_OBJECT
	Q_PROPERTY(QColor disableColor WRITE SetDisableColor READ GetDisableColor)
	Q_PROPERTY(QColor normalColor WRITE SetNormalColor READ GetNormalColor)

public:
	explicit MainWindowSettingButton(QWidget *parent = Q_NULLPTR);
	~MainWindowSettingButton() {}

	void SetIcon(QPixmap icon) { m_paintIcon = icon; }
	void SetIconGeo(QRect geo) { m_iconGeo = geo; }

private:
	QRect m_iconGeo;

protected:
	QPixmap m_paintIcon;

	virtual void showEvent(QShowEvent* e) override;
	virtual void paintEvent(QPaintEvent *e) override;
	virtual void changeEvent(QEvent *e) override;

	// q-property
	QColor m_disableColor;
	QColor m_normalColor = Qt::black;

	void SetDisableColor(QColor color) { m_disableColor = color; }
	QColor GetDisableColor() { return m_disableColor; }
	void SetNormalColor(QColor color);
	QColor GetNormalColor() { return m_normalColor; }
};

class MutePushButton : public MainWindowSettingButton
{
public:
	explicit MutePushButton(QWidget *parent = Q_NULLPTR);
	~MutePushButton() {}

	void SetMuted(bool mute, bool disable);

private:
	QPixmap m_muteOff;
	QPixmap m_muteOn;
};

class ViewerPushButton : public MainWindowSettingButton
{
public:
	explicit ViewerPushButton(QWidget *parent = Q_NULLPTR);
	~ViewerPushButton() {}
	
	bool SetViewerCount(int viewer);

private:
	QString		m_viewPrintNum;

	virtual void paintEvent(QPaintEvent *e) override;
	virtual void showEvent(QShowEvent* e) override;

protected:
	void SetDisabled(bool disable);
};