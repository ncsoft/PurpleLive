#include "MaskSettingWidget.h"

#include "StartUpWindow.h"
#include "MainWindow.h"
#include "VideoChatApp.h"
#include "LocalSettings.h"
#include "AppString.h"

MaskSettingWidget::MaskSettingWidget(QWidget *parent)
	: StartUpPopUp(parent)
{
	ui.setupUi(this);

	SetChildWidget(ui.mainWidget);

	ui.mask_listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
}

MaskSettingWidget::~MaskSettingWidget()
{
}

void MaskSettingWidget::on_maskEnable_checkBox_stateChanged(int state)
{
	if (state == Qt::Checked) {
		m_currentChecked = true;
	}
	else if (state == Qt::Unchecked) {
		m_currentChecked = false;
	}

	SetEnableMaskItem(m_currentChecked);
}

void MaskSettingWidget::SetEnableMaskItem(bool enable)
{
	for (int i = 0; i < ui.mask_listWidget->count(); i++)
	{
		QListWidgetItem *item = ui.mask_listWidget->item(i);
		MaskSettingListItem* itemWidget = static_cast<MaskSettingListItem*>(ui.mask_listWidget->itemWidget(item));

		itemWidget->SetMaskEnable(enable);
		itemWidget->GetLabel()->setEnabled(enable);
	}
}

void MaskSettingWidget::SetInitMaskEnable(bool enable)
{
	m_lastChecked = enable;
	m_currentChecked = m_lastChecked;

	if (enable) {
		on_maskEnable_checkBox_stateChanged(Qt::Checked);
	}
	else {
		on_maskEnable_checkBox_stateChanged(Qt::Unchecked);
	}

	ui.maskEnable_checkBox->setChecked(enable);
}

void MaskSettingWidget::HideEventCalled()
{
	MainWindow::Get()->GetStartupWindow()->SaveMaskInfotoLocal(false);
}

void MaskSettingWidget::ShowEventCalled()
{

}

int MaskSettingWidget::GetListWidgetHeight()
{
	return m_itemCount * MASK_SET_ITEM_HEIGHT + ERASE_SCROLL_MARGIN;
}

void MaskSettingWidget::SetMaskItemCount(int itemCount)
{
	m_itemCount = itemCount;
}

void MaskSettingWidget::on_popup_close_btn_clicked()
{
	on_cancel_btn_clicked();
}

void MaskSettingWidget::on_cancel_btn_clicked()
{
	MainWindow::Get()->GetStartupWindow()->SaveMaskInfotoLocal(false);
	StartUpPopUp::hide();
}

void MaskSettingWidget::on_select_btn_clicked()
{
	MainWindow::Get()->GetStartupWindow()->SaveMaskInfotoLocal(true);
	StartUpPopUp::hide();
}

void MaskSettingWidget::SaveLocal(bool save)
{
	if (save) {
		m_lastChecked = m_currentChecked;
		LocalSettings::SetEnableGameMask(m_lastChecked);
	}

	ui.maskEnable_checkBox->setChecked(m_lastChecked);
}

int MaskSettingWidget::GetStartUpPopUpY() 
{ 
	return (MainWindow::Get()->GetStartupWindow()->height() - MainWindow::Get()->GetStartupWindow()->getTitlebarHeight() - GetStartUpPopUpHeight()) / 2 + MainWindow::Get()->GetStartupWindow()->getTitlebarHeight();
}

int MaskSettingWidget::GetStartUpPopUpHeight() 
{ 
	int newh = GetListWidgetHeight() + STARTPOPUP_CHILD_TOP_BOTTOM_MARGIN + MASKSET_LABEL_HEIGHT;

	if (newh > MainWindow::Get()->GetStartupWindow()->height() - MainWindow::Get()->GetStartupWindow()->getTitlebarHeight())
	{
		newh = MainWindow::Get()->GetStartupWindow()->height() - MainWindow::Get()->GetStartupWindow()->getTitlebarHeight() - STARTPOPUP_Y_MARGIN * 2;
	}

	return newh;
}

void MaskSettingWidget::UpdateLanguageStringCalled()
{
	SetLabel(AppString::GetQString("SlideUpDlg/mask_set_title"));
	ui.mask_label->setText(AppString::GetQString("SlideUpDlg/mask_set_label"));
}