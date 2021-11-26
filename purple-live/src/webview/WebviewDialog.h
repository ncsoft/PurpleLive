#pragma once

#include <QDialog>
#include "LocalSettings.h"
#include "AppUtils.h"
#include "WebviewConfig.h"

class WebviewDialog : public QDialog
{
public:
	explicit WebviewDialog(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags()) : QDialog{parent, f} 
	{ 
		
	}

private:
	void initGeometry()
	{
		QRect default_rect = AppUtils::GetScreenCenterGeometry(this, WebviewConfig::default_width, WebviewConfig::default_height);
		QRect saved_rect = LocalSettings::GetWindowGeometry(metaObject()->className(), default_rect);
		if (!AppUtils::VerifyWindowRect(this, saved_rect))
		{
			saved_rect = default_rect;
		}
		setGeometry(saved_rect);
	}

	void showEvent(QShowEvent *event)
	{
		initGeometry();
	}

	bool event(QEvent *event)
	{
		switch (event->type())
		{
		case QEvent::Show:
			{
				QRect default_rect = AppUtils::GetScreenCenterGeometry(this, WebviewConfig::default_width, WebviewConfig::default_height);
				QRect saved_rect = LocalSettings::GetWindowGeometry(metaObject()->className(), default_rect);
				if (!AppUtils::VerifyWindowRect(this, saved_rect))
				{
					saved_rect = default_rect;
				}
				setGeometry(saved_rect);
				m_init_geometry = true;
			}
			break;
		case QEvent::Move:
		case QEvent::Resize:
			{
				if (m_init_geometry)
				{
					LocalSettings::SetWindowGeometry(metaObject()->className(), geometry());
				}
			}
			break;
		}

		return QDialog::event(event);
	}

private:
	bool m_init_geometry = false;
};