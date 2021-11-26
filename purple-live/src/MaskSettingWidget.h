#pragma once

#include "StartUpPopUp.h"

#include "ui_MaskSettingWidget.h"

#define MASKSET_LABEL_HEIGHT	40
#define ERASE_SCROLL_MARGIN		20

class MaskSettingWidget : public StartUpPopUp
{
	Q_OBJECT

public:
	MaskSettingWidget(QWidget *parent = Q_NULLPTR);
	virtual ~MaskSettingWidget();

	void SetMaskItemCount(int itemCount);
	void SetInitMaskEnable(bool enable);
	void SetMaskDisable(bool disable) { ui.maskEnable_checkBox->setChecked(!disable); }
	bool GetMaskEnable() { return ui.maskEnable_checkBox->isChecked(); }
	QListWidget* GetListWidget() { return ui.mask_listWidget; }
	void SaveLocal(bool save);

private:
	Ui::MaskSettingWidget ui;
	int m_itemCount = 0;
	bool m_lastChecked = false;
	bool m_currentChecked = false;

	int GetListWidgetHeight();
	void SetEnableMaskItem(bool enable);

private slots:
	void on_maskEnable_checkBox_stateChanged(int state);
	void on_popup_close_btn_clicked();
	void on_cancel_btn_clicked();
	void on_select_btn_clicked();

protected:
	virtual void ShowEventCalled() override;
	virtual void HideEventCalled() override;
	virtual int GetStartUpPopUpY() override;
	virtual int GetStartUpPopUpHeight() override;
	virtual void UpdateLanguageStringCalled() override;
};