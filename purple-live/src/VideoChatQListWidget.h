#pragma once

#include <qwidget.h>
#include <qlistwidget.h>
#include <chrono>

#include "DimWidget.h"

class VideoChatQListWidget : public QListWidget
{
	Q_OBJECT

public:
	VideoChatQListWidget(QWidget *parent = Q_NULLPTR) {}
	~VideoChatQListWidget() {}
	void SetItemWidget(QListWidgetItem *item, QWidget *widget);

private:
	std::chrono::system_clock::time_point m_start;
};