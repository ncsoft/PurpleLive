#include "PreviewContainerWidget.h"
#include "MainWindow.h"
#include "PreviewWidget.h"
#include "easylogging++.h"
#include "AppUtils.h"

PreviewContainerWidget::PreviewContainerWidget(QWidget *parent)
	: QWidget(parent)
{
	
}

void PreviewContainerWidget::Resize()
{
	PreviewWidget* previewWidget = MainWindow::Get()->GetPreviewWidget();
	QSize sourceSize{AppConfig::Instance()->streaming_width, AppConfig::Instance()->streaming_height};
	RECT rc = AppUtils::GetAspectRatioRect(sourceSize.width(), sourceSize.height(), width(), height());

	previewWidget->move(rc.left + 1, rc.top);
	previewWidget->setFixedSize(rc.right-rc.left - 2, rc.bottom-rc.top);
}

void PreviewContainerWidget::resizeEvent(QResizeEvent * event)
{
	//LOG(DEBUG) << format_string("[PreviewContainerWidget::resizeEvent] size(%d %d)", width(), height());
	Resize();
}
