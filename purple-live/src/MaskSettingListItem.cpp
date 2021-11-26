#include "MaskSettingListItem.h"

#include "LocalSettings.h"
#include "VideoChatApp.h"
#include "MainWindow.h"
#include "StartUpWindow.h"

MaskSettingListItem::MaskSettingListItem(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
}

MaskSettingListItem::~MaskSettingListItem()
{

}

void MaskSettingListItem::on_hide_checkBox_stateChanged(int state)
{
	if (state == Qt::Checked) {
		m_currentChecked = true;
	}
	else if (state == Qt::Unchecked) {
		m_currentChecked = false;
	}
}

void MaskSettingListItem::SetMaskChecked(bool enable)
{ 
	m_lastChecked = enable;
	m_currentChecked = m_lastChecked;
	ui.hide_checkBox->setChecked(m_lastChecked);
}

void MaskSettingListItem::SaveLocal(bool save)
{
	if (save)
	{
		m_lastChecked = m_currentChecked;
		LocalSettings::SetGameMaskSettings(App()->GetOriginGameCode(), MainWindow::Get()->GetStartupWindow()->GetMaskItemCount(), m_index, m_lastChecked);
	}

	ui.hide_checkBox->setChecked(m_lastChecked);
}