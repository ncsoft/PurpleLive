#pragma once

#include <qwidget.h>

#include "ui_MaskSettingListItem.h"

class MaskSettingListItem : public QWidget
{
	Q_OBJECT

public:
	MaskSettingListItem(QWidget *parent = Q_NULLPTR);
	~MaskSettingListItem();

	QLabel* GetLabel() { return ui.maskName_label; }
	void SetIndex(int index) { m_index = index; }
	void SetMaskName(const QString& name) { ui.maskName_label->setText(name); }
	void SetMaskChecked(bool enable);
	void SetMaskEnable(bool enable) { ui.hide_checkBox->setEnabled(enable); }
	void SaveLocal(bool save);

private:
	Ui::MaskSettingListItem ui;
	int m_index = -1;
	bool m_currentChecked = false;
	bool m_lastChecked = false;

private slots:
	void on_hide_checkBox_stateChanged(int state);
};