#include "MicSettingListItem.h"
#include "VideoChatApp.h"
#include "AppString.h"
#include "AppUtils.h"
#include <qpainter.h>

MicSettingListItem::MicSettingListItem(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
}

MicSettingListItem::~MicSettingListItem()
{

}

void MicSettingListItem::SetMicInfo(AudioDeviceInfo* info)
{
	if (info)
	{
		SetMicName(StringUtils::Local8bitToUnicode(info->name.c_str()));
		SetMicID(StringUtils::Local8bitToUnicode(info->id.c_str()));
		SetMicDefault(info->is_default);
	}
	else
	{
		SetMicName(AppString::GetQString("Common/un_used"));
	}
}

void MicSettingListItem::SetIndex(const int index)
{ 
	m_index = index;
}

void MicSettingListItem::SetMicName(const QString& name)
{ 
	const int widthMargin = 48;
	QString _text = name;
	int _width = DrawUtils::GetFontWidth(m_micNameLabelFont, _text);

	if (_width + widthMargin > maximumWidth())
	{
		for (int i = _text.length(); i > 0; i--)
		{
			QString newName = _text.mid(0, i);
			newName += "...";
			int newWidth = DrawUtils::GetFontWidth(m_micNameLabelFont, newName);

			if (newWidth + widthMargin < maximumWidth())
			{
				_text = newName;
				break;
			}
		}
	}
	
	ui.micName_label->setText(_text);
}

void MicSettingListItem::SetMicID(const QString& name)
{
	m_micID = name;
}

void MicSettingListItem::paintEvent(QPaintEvent *e)
{
	QPainter painter(this);

	painter.setRenderHint(QPainter::Antialiasing);

	if (m_isSelected)
	{
		painter.setPen(QPen(GetSelectedCircleColor(), 5, Qt::SolidLine, Qt::SquareCap));
		painter.setBrush(QBrush(GetInnerCircle()));
		painter.drawEllipse(width() - (MICSETTING_SELECT_CIRCLE_SIZE + 10) + 1, height() / 2 - MICSETTING_SELECT_CIRCLE_SIZE / 2 + 1, MICSETTING_SELECT_CIRCLE_SIZE - 2, MICSETTING_SELECT_CIRCLE_SIZE - 2);
	}
	else
	{
		painter.setPen(QPen(GetNotSelectedCircleColor(), 1, Qt::SolidLine, Qt::SquareCap));
		painter.drawEllipse(width() - (MICSETTING_SELECT_CIRCLE_SIZE + 10), height() / 2 - MICSETTING_SELECT_CIRCLE_SIZE / 2, MICSETTING_SELECT_CIRCLE_SIZE, MICSETTING_SELECT_CIRCLE_SIZE);
		
	}

	painter.end();
}