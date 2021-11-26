#pragma once

#include <QWidget>

class MainWindowCentralWidget : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(QColor previewBackground WRITE SetPreviewBackground READ GetPreviewBackground)
	Q_PROPERTY(QColor previewBackgroundBorder WRITE SetPreviewBackgroundBorder READ GetPreviewBackgroundBorder)

public:
	MainWindowCentralWidget(QWidget *parent = Q_NULLPTR);
	~MainWindowCentralWidget() {}

protected:
	virtual void paintEvent(QPaintEvent *e) override;
	
private:
	// q-property value, func
	QColor m_previewBackground;
	QColor m_previewBackgroundBorder;

	void SetPreviewBackground(QColor color) { m_previewBackground = color; }
	QColor GetPreviewBackground() { return m_previewBackground; }
	void SetPreviewBackgroundBorder(QColor color) { m_previewBackgroundBorder = color; }
	QColor GetPreviewBackgroundBorder() { return m_previewBackgroundBorder; }
};

