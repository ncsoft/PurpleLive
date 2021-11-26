#pragma once

#include <qwidget.h>
#include <QPointer>
#include <qtimer.h>

#include "AppConfig.h"
#include "AppUtils.h"
#include "ui_SettingPopUp.h"

#include "QFramelessPopup.h"
#include "OpenLicensePopup.h"
#include "StartUpWindow.h"
#include "MicSlider.h"

#define		SETTINGPOPUP_HEIGHT							408				// px, height(고정 값)
#define		SETTINGPOPUP_WIDTH							254				// px, width(고정 값)
#define		SETTINGPOPUP_CHECKBOX_Y_MARGIN				(20 + 16)		//	px
#define		SETTINGPOPUP_CHECKBOX_MAXIMUM_HEIGHT		128				//	px

#define		MIC_INPUT_LEVEL_REPAINT_PERIOD				10				// ms

class SettingPopMicListComboBoxItem;

class SettingPopUp : public QFramelessPopup
{
	Q_OBJECT
	Q_PROPERTY(QColor qcomboBorder WRITE SetQcomboBorder READ GetQcomboBorder)
	Q_PROPERTY(QColor qcomboBackground WRITE SetQcomboBackground READ GetQcomboBackground)
	Q_PROPERTY(QColor highLightedText WRITE SetHighLightedText READ GetHighLightedText)
	Q_PROPERTY(QColor nonHighLightedText WRITE SetNonHighLightedText READ GetNonHighLightedText)

public:
	SettingPopUp(QWidget *parent = Q_NULLPTR);
	~SettingPopUp();
	
	int		getSettingPopUpWidth();
	int		getSettingPopUpHeight();
	void	Init(std::string game_code);
	void	Show();

	void	setHideInfoChecked(bool _b) { ui.hide_info_checkBox->setChecked(_b); }
	void	setCheckBoxLabelVec(std::vector<QWidget*>& _checkVector);
	int		GetMicListComboBoxHighlight() { return m_miclistComboboxHightedIndex; }
	void	SetMicInputLevel(unsigned int level);
	int		GetMicListComboboxWidth() { return ui.settingpop_mic_combobox->width(); }
	void	SetLocalSavedMicInfo(int volume, bool muted);
	void	SaveLocalMicInfo();
	void	SetMicDisable(bool disable);
	Qt::CheckState	GetMuted() { return ui.micMute_checkBox->checkState(); }

public slots:
	virtual void hide();
	void	on_micMute_checkBox_stateChanged(int state);
	void	on_hide_info_checkBox_clicked(bool _bSetHide);
	bool	eventFilter(QObject* obj, QEvent* event);
	void	on_opensource_btn_clicked();
	void	SliderValueChanged(int value);

private:
	Ui::SettingPopUp ui;
	QWidget* m_parent;
	QPointer<dimmingWidget>	m_dimWidget = Q_NULLPTR;
	QPointer<OpenLicensePopup>	m_popenLicensePopup = nullptr;
	SettingPopMicListComboBoxItem* m_pMicComboItemdel;
	QTimer m_micInputLevelPaintTimer;
	std::vector<std::wstring> m_maskingAreaVec;
	std::vector<QLabel*> m_comboLabel;
	std::vector<QCheckBox*> m_checkVec;
	std::string m_game_code;
	vector<AudioDeviceInfo> m_micInfoList;
	AudioDeviceInfo m_currSelectedMicDevice;
	int m_miclistComboboxHightedIndex;
	int m_lastMicVolume = 0;
	bool m_micListChanged = false;

	void setUIPosition();
	void SetMicList();
	bool event(QEvent *event);
	bool setUIText(std::string _local);
	void StartPaintInputMicLevel(bool startPaint);
	AudioDeviceInfo GetEmptyMicInfo();
	virtual const int getTitlebarHeight() { return 0; }
	virtual const int getTitlebarWidth() { return 0; }
	
private slots:
	void MiclistComboBoxhighlighted(int index);
	void MicInputRepaintTimerCall();
	void on_settingpop_mic_combobox_currentIndexChanged(int index);

signals:
	void SetSliderMicLevel(unsigned int level);
	int GetSliderMicLevel();

private:
	// q-property value, func
	QColor m_qcomboBorder;
	QColor m_qcomboBackground;
	QColor m_highLightedText;
	QColor m_nonHighLightedTextColor;

	void SetQcomboBorder(QColor color);
	void SetQcomboBackground(QColor color);
	void SetHighLightedText(QColor color);
	void SetNonHighLightedText(QColor color);

	QColor GetQcomboBorder() { return m_qcomboBorder; }
	QColor GetQcomboBackground() { return m_qcomboBackground; }
	QColor GetHighLightedText() { return m_highLightedText; }
	QColor GetNonHighLightedText() { return m_nonHighLightedTextColor; }
};