#include "CharacterListItem.h"
#include "VideoChatApp.h"
#include "AppUtils.h"

#include <qpainter.h>

CharacterListItem::CharacterListItem(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
}

CharacterListItem::~CharacterListItem()
{

}

void CharacterListItem::SetCharacterName(const QString& name)
{
	ui.characterName_label->setText(DrawUtils::ChangeDottedStr(QString(name), m_nameMaxWidth, m_labelTextFont));
}

void CharacterListItem::SetServerName(const QString& name)
{
	ui.server_label->setText(DrawUtils::ChangeDottedStr(QString(name), m_serverMaxWidth, m_labelTextFont));
}

void CharacterListItem::SetIndex(const int index)
{ 
	m_index = index;
	m_defaultCharacterImage = QPixmap(":/StartStreamDlg/image/ic_default_character.png");
	SetCharacterName(QString::fromStdString(LimeCommunicator::Instance()->GetCharacterNamefromIndex(index)));
	SetServerName(QString::fromStdString(LimeCommunicator::Instance()->GetCharacterServerNamefromIndex(index)));
}

void CharacterListItem::paintEvent(QPaintEvent *e)
{
	QPainter painter(this);

	painter.setRenderHint(QPainter::Antialiasing);

	if (m_characterImage.isNull() == false)
	{
		DrawImage(&m_characterImage, &painter, 8, 8, m_imageSize, m_imageSize);
	}
	else
	{
		DrawUtils::DrawEllipseImage(&m_defaultCharacterImage, &painter, QRect(8, 8, m_imageSize, m_imageSize));
	}

	if (m_isSelected)
	{
		// Select Circle
		painter.setPen(QPen(GetSelectedCircleColor(), 5, Qt::SolidLine, Qt::SquareCap));
		painter.setBrush(QBrush(GetInnerCircle()));
		painter.drawEllipse(width() - (m_circleSize + 14) + 1, height() / 2 - m_circleSize / 2 + 1, m_circleSize - 2, m_circleSize - 2);
	}
	else
	{
		painter.setPen(QPen(GetNotSelectedCircleColor(), 1, Qt::SolidLine, Qt::SquareCap));
		painter.drawEllipse(width() - (m_circleSize + 14), height() / 2 - m_circleSize / 2, m_circleSize, m_circleSize);
	}

	painter.end();
}

void CharacterListItem::DrawImage(QPixmap* pixmap, QPainter* painter, int _x, int _y, int _width, int _height)
{
	QPixmap scaled = pixmap->scaled(_width, _height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	QImage fixedImage(_width, _height, QImage::Format_ARGB32_Premultiplied);
	fixedImage.fill(0);
	QPainter imgPainter(&fixedImage);
	QPainterPath clip;
	clip.addRect(0, 0, _width, _height);
	imgPainter.setClipPath(clip);
	imgPainter.drawPixmap(0, 0, _width, _height, scaled);
	imgPainter.end();

	painter->drawPixmap(_x, _y, _width, _height, QPixmap::fromImage(fixedImage));
}

QPixmap* CharacterListItem::GetCharacterImage() 
{
	if (m_characterImage.isNull() == false)
	{
		return &m_characterImage;
	}
	
	return nullptr;
}