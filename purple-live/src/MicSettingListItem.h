#pragma once

#include <qwidget.h>
#include "AudioMixer/NCAudioMixer.h"

#include "ui_MicSettingListItem.h"

#define MICSETTING_SELECT_CIRCLE_SIZE		20

class MicSettingListItem : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(QColor selectedCircle WRITE SetSelectedCircleColor READ GetSelectedCircleColor)
	Q_PROPERTY(QColor innerCircle WRITE SetInnerCircle READ GetInnerCircle)
	Q_PROPERTY(QColor notSelectedCircle WRITE SetNotSelectedCircleColor READ GetNotSelectedCircleColor)
	Q_PROPERTY(QFont micNameLabelFont WRITE SetMicNameLabelFont READ GetMicNameLabelFont)

public:
	MicSettingListItem(QWidget *parent = Q_NULLPTR);
	~MicSettingListItem();

	void SetIndex(const int index);
	const int GetIndex() { return m_index; }
	void SetSelected(const bool b) { m_isSelected = b; }
	void SetMicName(const QString& name);
	void SetMicID(const QString& name);
	void SetMicDefault(bool defualt) { m_isDefault = defualt; }
	void SetMicInfo(AudioDeviceInfo* info);
	const QString GetMicName() { return ui.micName_label->text(); }
	const QString& GetMicID() { return m_micID; }
	const bool GetDefault() { return m_isDefault; }

private:
	Ui::MicSettingListItem ui;
	bool m_isSelected = false;
	bool m_isDefault = false;
	int m_index = -1;
	QString m_micID = "";

	void paintEvent(QPaintEvent *e);

	// q-property value, func
	QColor m_selectedCircle;
	QColor m_innerCircle;
	QColor m_notSelectedCircle;
	QFont  m_micNameLabelFont;

	void SetSelectedCircleColor(QColor color) { m_selectedCircle = color; }
	void SetInnerCircle(QColor color) { m_innerCircle = color; }
	void SetNotSelectedCircleColor(QColor color) { m_notSelectedCircle = color; }
	void SetMicNameLabelFont(QFont font) { m_micNameLabelFont = font; }

	QColor GetSelectedCircleColor() { return m_selectedCircle; }
	QColor GetInnerCircle() { return m_innerCircle; }
	QColor GetNotSelectedCircleColor() { return m_notSelectedCircle; }
	QFont  GetMicNameLabelFont() { return m_micNameLabelFont; }
};