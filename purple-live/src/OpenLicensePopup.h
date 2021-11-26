#pragma once

#include <QWidget>
#include <qpointer.h>
#include <qnetworkreply.h>

#include "ui_OpenLicensePopup.h"
#include "QFramelessDialog.h"
#include "DimWidget.h"

#define LICENSEPOPUP_MAX_WIDTH		440
#define LICENSEPOPUP_MIN_WIDTH		320
#define LICENSEPOPUP_MAX_HEIGHT		536
#define LICENSEPOPUP_MIN_HEIGHT		336
#define POPUP_XY_MARGIN				24
#define SHADOW_PADDING_X			10
#define SHADOW_PADDING_Y			25

class OpenLicensePopup : public QFramelessDialog
{
	Q_OBJECT

public:
	OpenLicensePopup(QWidget *parent = Q_NULLPTR);
	~OpenLicensePopup();

	void Init();
	void Show();

private:
	Ui::OpenLicensePopup ui;
	void InitUI();
	QPointer<QWidget>			m_parent = Q_NULLPTR;
	QPointer<dimmingWidget>		m_dimWidget = Q_NULLPTR;

	QString GetAppVersion();
	void UpdateLicenseText();
	void SetDimWidget(bool enable);
	void SetFixedSize(int _width, int _height);
	virtual const int getTitlebarHeight() { return 0; }
	virtual const int getTitlebarWidth() { return 0; }

private slots:
	void on_openLisencePopCloseBtn_clicked();
	void on_ok_btn_clicked();
	void LicenseTextDownFinished(QNetworkReply *reply);
	void showContextMenu(const QPoint &pt);
	void ResetGeometry();
	void OnScreenLockShowEvent();
	void OnScreenLockHideEvent();
	void UpdateLanguageString();
};
