#pragma once

#include <QDialog>
#include "ui_StartUpLauncherWindow.h"
#include <string>

#include "QFramelessDialog.h"

class StartUpLauncherWindow : public QFramelessDialog
{
	Q_OBJECT

public:
	StartUpLauncherWindow(QWidget *parent = Q_NULLPTR);
	~StartUpLauncherWindow();

	bool GetActive();
	void SetActive(bool enable);

	void		UpdateGameList();
	void		AddThumbnail(const std::wstring appName, HWND hwnd);

private:
	Ui::StartUpLauncherWindow ui;

	bool m_active = false;
	bool m_changed_size_or_pos = false;

	QPixmap		GetCapturePixmap(HWND hwnd);

	virtual const int getTitlebarHeight() { return ui.widget_titlebar->height(); }
	virtual const int getTitlebarWidth() { return ui.widget_titlebar->width(); }

	bool nativeEvent(const QByteArray &eventType, void *message, long *result);

public slots:
	bool		event(QEvent *event);
	void		on_listWidget_window_itemDoubleClicked(QListWidgetItem *item);
	void		on_pushButton_start_clicked();
};
