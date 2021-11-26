#pragma once

#include "StartUpPopUp.h"

#include "ui_MicSettingWidget.h"

#define		MICSET_LABEL_MARGIN				(40*3)
#define		MICSET_HANDLE_WIDTH				8
#define		MICSET_HANDLE_HEIGHT			16
#define		MICSET_SLIDER_ROUND_MARGIN		3
#define		MICSET_ITEM_HEIGHT				40
#define		HANDLE_SHADOW_BLUR_RADIUS		8.0
#define		HANDLE_SHADOW_DISTANCE			4.0
#define		HANDLE_SHADOW_COLOR				QColor(0, 0, 0, 255 * 0.04)

class MicSettingWidget : public StartUpPopUp
{
	Q_OBJECT
	Q_PROPERTY(QColor subpageColor WRITE SetSubpageColor READ GetSubpageColor)
	Q_PROPERTY(QColor addpageColor WRITE SetAddpageColor READ GetAddpageColor)
	Q_PROPERTY(QColor handleColor WRITE SetHandleColor READ GetHandleColor)

public:
	MicSettingWidget(QWidget *parent = Q_NULLPTR);
	virtual ~MicSettingWidget();

	QString GetMicNameFromIndex(int index);
	QListWidget* GetListWidget() { return ui.mic_listWidget; }
	void SetSelectedIndex(int index);
	void SetVolume(float volume) { ui.micVolume_slider->setValue(volume * 100); }
	void SetMute(bool mute) { ui.mute_checkbox->setChecked(mute); }
	const int GetSelectedIndex() { return m_selectedIndex; }
	const int GetItemCount() { return ui.mic_listWidget->count(); }
	const bool GetMuted() { return ui.mute_checkbox->isChecked(); }
	const float GetVolume() { return float(ui.micVolume_slider->value()) / 100.0; }

public slots:
	void show();

private:
	Ui::MicSettingWidget ui;
	int m_lastSelectedIndex = -1;
	int m_selectedIndex = -1;
	int m_lastMicVolume = 0;
	bool m_initWidget = false;
	bool m_usedCapture = false;
	bool m_sliderEnter = false;

	void SliderWidgetEnter(bool enter);
	bool CheckUseCapture();
	void SetCurrentIndex(int index);
	void DrawHandleDownShadow(QPainter* painter, QRect& handle);

private slots:
	bool eventFilter(QObject* obj, QEvent* event);
	void ItemRowChanged(int row);
	void on_mute_checkbox_stateChanged(int state);
	void SliderValueChanged(int value);
	void on_popup_close_btn_clicked();
	void on_cancel_btn_clicked();
	void on_select_btn_clicked();

protected:
	virtual void ShowEventCalled() override;
	virtual void HideEventCalled() override;
	virtual int GetStartUpPopUpY() override;
	virtual int GetStartUpPopUpHeight() override;
	virtual void UpdateLanguageStringCalled() override;

private:
	// q-property value, func
	QColor m_subpageColor;
	QColor m_addpageColor;
	QColor m_handleColor;

	void SetSubpageColor(QColor color) { m_subpageColor = color; }
	void SetAddpageColor(QColor color) { m_addpageColor = color; }
	void SetHandleColor(QColor color) { m_handleColor = color; }

	QColor GetSubpageColor() { return m_subpageColor; }
	QColor GetAddpageColor() { return m_addpageColor; }
	QColor GetHandleColor() { return m_handleColor; }
};