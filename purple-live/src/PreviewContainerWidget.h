#pragma once

#include <QWidget>

class PreviewContainerWidget : public QWidget
{
	Q_OBJECT

public:
	PreviewContainerWidget(QWidget *parent = Q_NULLPTR);

	void	Resize();

protected:
	void	resizeEvent(QResizeEvent* event) override;
	
};
