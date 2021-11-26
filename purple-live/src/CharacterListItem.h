#pragma once

#include <qwidget.h>

#include "ui_CharacterListItem.h"

class CharacterListItem : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(QColor selectedCircle WRITE SetSelectedCircleColor READ GetSelectedCircleColor)
	Q_PROPERTY(QColor innerCircle WRITE SetInnerCircle READ GetInnerCircle)
	Q_PROPERTY(QColor notSelectedCircle WRITE SetNotSelectedCircleColor READ GetNotSelectedCircleColor)
	Q_PROPERTY(int circleSize WRITE SetCircleSize READ GetCircleSize)
	Q_PROPERTY(int imageSize WRITE SetImageSize READ GetImageSize)
	Q_PROPERTY(int nameMaxWidth WRITE SetNameMaxWidth READ GetNameMaxWidth)
	Q_PROPERTY(int serverMaxWidth WRITE SetServerMaxWidth READ GetServerMaxWidth)
	Q_PROPERTY(QFont labelTextFont WRITE SetLabelTextFont READ GetLabelTextFont)

public:
	CharacterListItem(QWidget *parent = Q_NULLPTR);
	~CharacterListItem();

	void SetIndex(const int index);
	const int GetIndex() { return m_index; }
	void SetSelected(const bool b) { m_isSelected = b; }
	void SetCharacterName(const QString& name);
	void SetServerName(const QString& name);
	void SetCharacterImage(const QPixmap image) { m_characterImage = image; }
	QPixmap* GetCharacterImage();

private:
	Ui::CharacterListItem ui;
	bool m_isSelected = false;
	int m_index = -1;
	QPixmap m_characterImage{};
	QPixmap m_defaultCharacterImage{};

	void paintEvent(QPaintEvent *e);
	void DrawImage(QPixmap* pixmap, QPainter* painter, int _x, int _y, int _width, int _height);

	// q-property value, func
	QColor m_selectedCircle;
	QColor m_innerCirle;
	QColor m_notSelectedCircle;
	int m_circleSize;
	int m_imageSize;
	int m_nameMaxWidth;
	int m_serverMaxWidth;
	QFont m_labelTextFont;

	void SetSelectedCircleColor(QColor color) { m_selectedCircle = color; }
	QColor GetSelectedCircleColor() { return m_selectedCircle; }
	void SetNotSelectedCircleColor(QColor color) { m_notSelectedCircle = color; }
	QColor GetNotSelectedCircleColor() { return m_notSelectedCircle; }
	void SetInnerCircle (QColor color) { m_innerCirle = color; }
	QColor GetInnerCircle() { return m_innerCirle; }
	void SetCircleSize(int val) { m_circleSize = val; }
	int GetCircleSize() { return m_circleSize; }
	void SetImageSize(int val) { m_imageSize = val; }
	int GetImageSize() { return m_imageSize; }
	void SetNameMaxWidth(int val) { m_nameMaxWidth = val; }
	int GetNameMaxWidth() { return m_nameMaxWidth; }
	void SetServerMaxWidth(int val) { m_serverMaxWidth = val; }
	int GetServerMaxWidth() { return m_serverMaxWidth; }
	void SetLabelTextFont(QFont val) { m_labelTextFont = val; }
	QFont GetLabelTextFont() { return m_labelTextFont; }
};