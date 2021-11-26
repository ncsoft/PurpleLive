#include "CharacterSelectWidget.h"

#include "MainWindow.h"
#include "LimeCommunicator.h"
#include "CharacterListItem.h"
#include "AppString.h"

CharacterSelectWidget::CharacterSelectWidget(QWidget *parent)
	: StartUpPopUp(parent)
{
	ui.setupUi(this);

	SetChildWidget(ui.CharacterSelectListWidget);

	connect(ui.CharacterSelectListWidget, QOverload<int>::of(&QListWidget::currentRowChanged), this, QOverload<int>::of(&CharacterSelectWidget::ItemRowChanged));

	ui.CharacterSelectListWidget->setAttribute(Qt::WA_TranslucentBackground);
}

CharacterSelectWidget::~CharacterSelectWidget()
{
}

void CharacterSelectWidget::ItemRowChanged(int row)
{
	m_selectedIndex = row;
	MainWindow::Get()->GetStartupWindow()->SelectedCharacterChange(m_selectedIndex);
}

void CharacterSelectWidget::show()
{
	m_isSelectClick = false;
	m_lastSelectedIndex = LimeCommunicator::Instance()->GetCurrentCharacterIndex();
	GetListWidget()->setCurrentRow(m_lastSelectedIndex);
	StartUpPopUp::show();
}

void CharacterSelectWidget::hide()
{
	StartUpPopUp::hide();
}

void CharacterSelectWidget::on_popup_close_btn_clicked()
{
	on_cancel_btn_clicked();
}

void CharacterSelectWidget::on_cancel_btn_clicked()
{
	MainWindow::Get()->GetStartupWindow()->SelectedCharacterChange(m_lastSelectedIndex);
	hide();
}

void CharacterSelectWidget::on_select_btn_clicked()
{
	m_isSelectClick = true;

	hide();

	if (m_selectedIndex >= 0)
	{
		MainWindow::Get()->GetStartupWindow()->OnChangeCharactor(m_selectedIndex);
	}
	else
	{
		MainWindow::Get()->GetStartupWindow()->SelectedCharacterChange(m_lastSelectedIndex);
	}
}

void CharacterSelectWidget::HideEventCalled()
{
	if(!m_isSelectClick) MainWindow::Get()->GetStartupWindow()->SelectedCharacterChange(m_lastSelectedIndex);
}

void CharacterSelectWidget::ShowEventCalled()
{
}

int CharacterSelectWidget::GetListWidgetHeight()
{ 
	int newh = ui.CharacterSelectListWidget->count() * CHARACTER_ITEM_HEIGHT + CHARACTER_ERASE_SCROLLBAR_MARGIN;

	if (newh > CHARACTER_WIDGET_MAX_HEIGHT) 
	{
		newh = CHARACTER_WIDGET_MAX_HEIGHT;
	}
	else if (newh < CHARACTER_WIDGET_MIN_HEIGHT) 
	{
		newh = CHARACTER_WIDGET_MIN_HEIGHT;
	}
	else
	{
		newh += CHARACTER_BOTTOM_MARGIN;
	}

	return newh;
}

int CharacterSelectWidget::GetStartUpPopUpY()
{
	return (MainWindow::Get()->GetStartupWindow()->height() - MainWindow::Get()->GetStartupWindow()->getTitlebarHeight() - GetStartUpPopUpHeight()) / 2 + MainWindow::Get()->GetStartupWindow()->getTitlebarHeight();
}

int CharacterSelectWidget::GetStartUpPopUpHeight()
{
	int newh = GetListWidgetHeight() + STARTPOPUP_CHILD_TOP_BOTTOM_MARGIN;
	int maxh = MainWindow::Get()->GetStartupWindow()->height() - MainWindow::Get()->GetStartupWindow()->getTitlebarHeight() - STARTPOPUP_Y_MARGIN * 2;

	if (newh > maxh) return maxh;
	return newh;
}

void CharacterSelectWidget::UpdateLanguageStringCalled()
{
	SetLabel(AppString::GetQString("SlideUpDlg/select_charactor"));
	SetTextCancelBtn(AppString::GetQString("Common/cancel"));
	SetTextSelectBtn(AppString::GetQString("Common/confirm"));
}