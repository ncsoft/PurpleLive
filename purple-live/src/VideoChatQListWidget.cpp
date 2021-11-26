#include "VideoChatQListWidget.h"
#include <qcoreapplication.h>

using namespace std;

void VideoChatQListWidget::SetItemWidget(QListWidgetItem *item, QWidget *widget)
{
	int _index = row(item);

	if (_index == 0) {
		m_start = std::chrono::system_clock::now();
	}

	std::chrono::system_clock::time_point _end = std::chrono::system_clock::now();

	if ((_end - m_start) >= CIRCLE_INDICATOR_REPAINT_INTERVAL)
	{
		m_start = _end;
		QCoreApplication::processEvents();
	}
	
	QListWidget::setItemWidget(item, widget);
}