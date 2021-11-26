#pragma once

#include "StartUpPopUp.h"

#include "ui_CharacterSelectWidget.h"

#define	CHARACTER_ITEM_HEIGHT					48
#define	CHARACTER_BOTTOM_MARGIN					8
#define CHARACTER_ERASE_SCROLLBAR_MARGIN		15
#define CHARACTER_WIDGET_MAX_HEIGHT				376
#define CHARACTER_WIDGET_MIN_HEIGHT				176

class CharacterSelectWidget : public StartUpPopUp
{
	Q_OBJECT

public:
	CharacterSelectWidget(QWidget *parent = Q_NULLPTR);
	virtual ~CharacterSelectWidget();

	VideoChatQListWidget* GetListWidget() { return ui.CharacterSelectListWidget; }
	int GetListWidgetHeight();

public slots:
	void show();
	void hide();

private:
	Ui::CharacterSelectWidget ui;
	int m_lastSelectedIndex = 0;
	int m_selectedIndex = 0;
	bool m_isSelectClick = false;

private slots:
	void ItemRowChanged(int row);
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