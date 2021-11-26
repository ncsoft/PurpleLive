#pragma once

#include <qcheckbox.h>

class MuteCheckBox : public QCheckBox {
	Q_OBJECT
	Q_PROPERTY(QColor iconColor WRITE SetIconColor READ GetIconColor)

public:
	explicit MuteCheckBox(QWidget *parent = Q_NULLPTR);
	~MuteCheckBox() {}

private:
	QPixmap m_muteOnIcon;
	QPixmap m_muteOffIcon;

	virtual void paintEvent(QPaintEvent *e) override;

	// q-property value
	QColor m_iconColor;

	void SetIconColor(QColor color);
	QColor GetIconColor() { return m_iconColor; }
};
