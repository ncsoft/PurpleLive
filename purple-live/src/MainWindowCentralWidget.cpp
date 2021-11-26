#include "MainWindowCentralWidget.h"

#include <QPainter>

MainWindowCentralWidget::MainWindowCentralWidget(QWidget *parent)
	: QWidget(parent)
{
}

void MainWindowCentralWidget::paintEvent(QPaintEvent * e)
{
	QPainter painter(this);

	painter.setBrush(QBrush(m_previewBackground));
	painter.setPen(QPen(m_previewBackground));
	painter.drawRect(rect());

	painter.setPen(QPen(m_previewBackgroundBorder));
	painter.drawLine(0, 0, 0, height());
	painter.drawLine(width()-1, 0, width()-1, height());
	
	painter.end();

	QWidget::paintEvent(e);
}
