#include "SettingPopUp.h"

#include "MaskingManager.h"
#include "AppConfig.h"
#include "AppString.h"
#include "LocalSettings.h"
#include "MainWindow.h"
#include "VideoChatApp.h"
#include "LocalSettings.h"
#include "ComboBoxItem.h"

#include <qsettings.h>
#include <qdesktopservices.h>
#include <qurl.h>
#include <qstyle.h>
#include <qabstractitemview.h>
#include <qpainter.h>
#include <qevent.h>

SettingPopUp::SettingPopUp(QWidget *parent)
	: QFramelessPopup(parent), m_parent(parent)
{
	ui.setupUi(this);

	if (!setUIText(AppConfig::default_locale)) {
		// UI 로드 실패
	}
}

SettingPopUp::~SettingPopUp()
{
	if (m_popenLicensePopup) {
		delete m_popenLicensePopup;
		m_popenLicensePopup = nullptr;
	}

	if (m_dimWidget)
	{
		delete m_dimWidget;
		m_dimWidget = nullptr;
	}

	if (m_pMicComboItemdel) delete m_pMicComboItemdel;
}

bool SettingPopUp::setUIText(std::string _local)
{
	ui.divier_line->setFixedHeight(1);
	ui.opensource_divider_line->setFixedHeight(1);
	ui.mic_divider_line->setFixedHeight(1);

	ui.hide_info_checkBox->setChecked(true);

	ui.hideinfo_label->setText(AppString::GetQString("SettingPopUp/hide_info_label"));
	ui.opensource_btn->setText(AppString::GetQString("SettingPopUp/open_source_btn"));
	ui.mic_label->setText(AppString::GetQString("SettingPopUp/mic_label"));

	m_pMicComboItemdel = new SettingPopMicListComboBoxItem(ui.settingpop_mic_combobox);
	ui.settingpop_mic_combobox->setItemDelegate(m_pMicComboItemdel);

	ui.settingpop_mic_combobox->connect(ui.settingpop_mic_combobox, QOverload<int>::of(&QComboBox::highlighted), this, &SettingPopUp::MiclistComboBoxhighlighted);
	ui.settingpop_mic_combobox->view()->window()->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
	connect(ui.mic_Slider, &QSlider::valueChanged, this, &SettingPopUp::SliderValueChanged);

	ui.mic_Slider->installEventFilter(this);
	ui.mic_Slider->setMaximum(99);
	ui.mic_Slider->setTickInterval(1);
	ui.mic_Slider->setValue(ui.mic_Slider->maximum());

	m_micInputLevelPaintTimer.setInterval(MIC_INPUT_LEVEL_REPAINT_PERIOD);
	connect(&m_micInputLevelPaintTimer, &QTimer::timeout, this, &SettingPopUp::MicInputRepaintTimerCall);

	connect(this, QOverload<unsigned int>::of(&SettingPopUp::SetSliderMicLevel), ui.mic_Slider, QOverload<unsigned int>::of(&MicSlider::SetMicLevel));
	connect(this, &SettingPopUp::GetSliderMicLevel, ui.mic_Slider, &MicSlider::GetMicLevel);
	
	return true;
}

void SettingPopUp::MicInputRepaintTimerCall()
{
	ui.mic_Slider->repaint();
}

void SettingPopUp::SliderValueChanged(int value)
{
	bool micCheckBoxChecked = ui.micMute_checkBox->isChecked();

	if (micCheckBoxChecked)
	{
		// Muted Checked
		if (value != 0) {
			ui.micMute_checkBox->setChecked(false);
		}
	}
	else
	{
		// Muted UnChecked
		if (value == 0) {
			ui.micMute_checkBox->setChecked(true);
		}
	}

	// default slider-value 0 ~ 99
	static_cast<MainWindow*>(App()->GetMainWindow())->SetMicVolume(float(value) / 100.0);
}

void SettingPopUp::SetMicList()
{
	MainWindow* pmainWindow = static_cast<MainWindow*>(App()->GetMainWindow());

	AudioDeviceInfo tempMic;
	vector<AudioDeviceInfo> tempMicList;

	pmainWindow->GetMicInfoList(tempMicList);

	tempMic.name = string(AppString::GetQString("ComboItemDelegate/error_cannot_find_mic").toLocal8Bit());
	tempMicList.push_back(tempMic);

	if (m_micInfoList.size() == tempMicList.size()) {
		for (int i = 0; i < m_micInfoList.size(); i++) {
			if (m_micInfoList[i].name != tempMicList[i].name) {
				m_micListChanged = true;
				break;
			}
		}
	}
	else {
		m_micListChanged = true;
	}

	m_micInfoList.clear();
	m_micInfoList.assign(tempMicList.begin(), tempMicList.end());

	if (m_micListChanged) {
		ui.settingpop_mic_combobox->clear();
		ui.settingpop_mic_combobox->addItem("");

		if (m_micInfoList.size() > 0) {
			for (int i = 0; i < m_micInfoList.size(); i++)
			{
				if (i == m_micInfoList.size() - 1) {
					m_currSelectedMicDevice = GetEmptyMicInfo();
					break;
				}

				ui.settingpop_mic_combobox->addItem("");

				// set current capture mic device
				if (m_micInfoList[i].is_capture) {
					m_currSelectedMicDevice = m_micInfoList[i];
					break;
				}
			}
			m_pMicComboItemdel->setitemcount(ui.settingpop_mic_combobox->count());
		}
		else {
			ui.settingpop_mic_combobox->setCurrentText(AppString::GetQString("ComboItemDelegate/error_cannot_find_mic"));
		}
	}
	else {
		for (int i = 0; i < m_micInfoList.size(); i++) {
			if (i == m_micInfoList.size() - 1) {
				m_currSelectedMicDevice = GetEmptyMicInfo();
				break;
			}

			if (m_micInfoList[i].is_capture) {
				if (m_currSelectedMicDevice.id != m_micInfoList[i].id || m_currSelectedMicDevice.name != m_micInfoList[i].name) {
					m_currSelectedMicDevice = m_micInfoList[i];
				}
				break;
			}
		}
	}
}

AudioDeviceInfo SettingPopUp::GetEmptyMicInfo()
{
	AudioDeviceInfo emptyMicInfo;
	emptyMicInfo.id = AppConfig::not_use_mic_device;
	return emptyMicInfo;
}

void SettingPopUp::MiclistComboBoxhighlighted(int index)
{
	m_miclistComboboxHightedIndex = index;
}

void SettingPopUp::on_settingpop_mic_combobox_currentIndexChanged(int index)
{
	if ((m_micInfoList.size() <= index) || (index < 0)) return;

	bool micIndexChanged = true;

	if (m_micListChanged) {
		
		m_micListChanged = false;

		for (int i = 0; i < m_micInfoList.size() - 1; i++) {
			if ((m_currSelectedMicDevice.name == m_micInfoList[i].name) && (m_currSelectedMicDevice.id == m_micInfoList[i].id))
			{
				index = i;
				m_currSelectedMicDevice = m_micInfoList[i];
				micIndexChanged = false;
			}
		}
	
		ui.settingpop_mic_combobox->setCurrentIndex(index);
	}

	MainWindow* pmainWindow = static_cast<MainWindow*>(App()->GetMainWindow());

	QString printtext = QString::fromLocal8Bit(m_micInfoList[index].name.c_str());
	ui.settingpop_mic_combobox->setCurrentText(printtext);

	bool bMicSelected = false;

	if (!micIndexChanged) return;

	if (m_micInfoList.size() != (index + 1)) {
		// 마이크 선택
		bMicSelected = true;
		pmainWindow->SetEnableInputAudioCapture(true);
		pmainWindow->SetMicSelect(index);
		m_currSelectedMicDevice = m_micInfoList[index];
		StartPaintInputMicLevel(true);
	}
	else {
		// 마이크 장비 없음 선택
		pmainWindow->SetEnableInputAudioCapture(false);
		pmainWindow->SetMicIndex(index);
		m_currSelectedMicDevice = GetEmptyMicInfo();
		StartPaintInputMicLevel(false);
	}

	SetMicDisable(!bMicSelected);
}

void SettingPopUp::StartPaintInputMicLevel(bool startPaint)
{
	if (startPaint) {
		m_micInputLevelPaintTimer.start();
	}
	else {
		m_micInputLevelPaintTimer.stop();
	}
	
	ui.mic_Slider->StopPaintInputVolume(!startPaint);
}

void SettingPopUp::on_micMute_checkBox_stateChanged(int state)
{
	MainWindow* pmainWindow = static_cast<MainWindow*>(App()->GetMainWindow());
	bool isMuted = false;

	if (state == Qt::Checked) {
		m_lastMicVolume = pmainWindow->GetMicVolume() * 100;
		ui.mic_Slider->setValue(0);
		isMuted = true;
	}
	else if (state == Qt::Unchecked) {
		ui.mic_Slider->setValue(m_lastMicVolume);
	}

	pmainWindow->SetMuted(isMuted);
	pmainWindow->update();
}

void SettingPopUp::SetLocalSavedMicInfo(int volume, bool muted)
{
	ui.mic_Slider->setValue(volume);
	m_lastMicVolume = volume;
	ui.micMute_checkBox->setChecked(muted);
	SetMicList();
}

void SettingPopUp::Init(std::string game_code)
{
	m_game_code = game_code;

	MaskingManager::Instance()->GetMaskNameList(m_maskingAreaVec);

	ui.checkWidget->setFixedWidth(width());
	int newhh = 20 + (m_maskingAreaVec.size() - 1) * SETTINGPOPUP_CHECKBOX_Y_MARGIN;
	ui.checkWidget->setFixedHeight(newhh);

	ui.main_widget->setFixedHeight(getSettingPopUpHeight());
	ui.main_widget->setFixedWidth(getSettingPopUpWidth());
	setFixedHeight(getSettingPopUpHeight());
	setFixedWidth(getSettingPopUpWidth());

	bool is_enable_game_mask = LocalSettings::GetEnableGameMask(LocalSettings::DEFAULT_ENABLE_GAME_MASK);
	QBitArray game_mask_settings = LocalSettings::GetGameMaskSettings(m_game_code, QBitArray(m_maskingAreaVec.size()));

	for (int i = 0; i < m_maskingAreaVec.size(); i++) {
		QLabel *label = new QLabel(ui.checkWidget);
		label->setObjectName(QString::number(i));
		m_comboLabel.push_back(label);

		QCheckBox* check = new QCheckBox(ui.checkWidget);
		check->setObjectName(QString::number(i));
		check->setChecked(game_mask_settings.at(i));
		check->setProperty("Style", check->isChecked() ? "0" : "1");
		check->setDisabled(!is_enable_game_mask);
		check->installEventFilter(this);
		m_checkVec.push_back(check);
	}

	for (int i = 0; i < m_maskingAreaVec.size(); i++) {
		m_comboLabel[i]->setText(QString::fromStdWString(m_maskingAreaVec[i]));
	}

	SetMicList();
}

void SettingPopUp::setUIPosition()
{
	for (int i = 0; i < m_comboLabel.size(); i++) {
		int wx = 16;
		int wy = (18 + 18) * i;

		m_comboLabel[i]->setGeometry(wx, wy, 200, 18);
		m_checkVec[i]->setGeometry(width() - 16 - 20, wy, 20, 20);

		m_comboLabel[i]->show();
		m_checkVec[i]->show();
	}
}

void SettingPopUp::Show()
{
	SetMicList();

	MainWindow* pmainWindow = static_cast<MainWindow*>(App()->GetMainWindow());

	if (pmainWindow) {
		int micIndex = pmainWindow->GetMicIndex();

		bool  bMicSelected = false;

		if (micIndex >= 0) {
			if (micIndex + 1 != ui.settingpop_mic_combobox->count()) {
				bMicSelected = true;
				ui.settingpop_mic_combobox->setCurrentIndex(micIndex);
				ui.settingpop_mic_combobox->setCurrentText(QString::fromLocal8Bit(m_micInfoList[micIndex].name.c_str()));
				StartPaintInputMicLevel(true);
			}
			else {
				// mic 선택안됨
				ui.settingpop_mic_combobox->setCurrentIndex(ui.settingpop_mic_combobox->count() - 1);
				ui.settingpop_mic_combobox->setCurrentText(AppString::GetQString("ComboItemDelegate/error_cannot_find_mic"));
				StartPaintInputMicLevel(false);
			}
		}
		else {
			// mic 선택안됨
			ui.settingpop_mic_combobox->setCurrentIndex(ui.settingpop_mic_combobox->count() - 1);
			ui.settingpop_mic_combobox->setCurrentText(AppString::GetQString("ComboItemDelegate/error_cannot_find_mic"));
			StartPaintInputMicLevel(false);
		}

		SetMicDisable(!bMicSelected);
	}

	// set Mask settings
	bool is_enable_game_mask = LocalSettings::GetEnableGameMask(LocalSettings::DEFAULT_ENABLE_GAME_MASK);
	QBitArray game_mask_settings = LocalSettings::GetGameMaskSettings(m_game_code, QBitArray(m_maskingAreaVec.size()));
	
	ui.hide_info_checkBox->setChecked(is_enable_game_mask);

	for (int i = 0; i < m_checkVec.size(); i++)
	{
		m_checkVec[i]->setChecked(game_mask_settings.at(i));
		ui.hide_info_checkBox->setChecked(is_enable_game_mask);
		m_checkVec[i]->setDisabled(!is_enable_game_mask);
	}

	setUIPosition();
	show();
}

void SettingPopUp::SetMicDisable(bool disable)
{
	MainWindow* pmainWindow = static_cast<MainWindow*>(App()->GetMainWindow());

	ui.mic_Slider->setDisabled(disable);
	ui.micMute_checkBox->setDisabled(disable);

	bool isMuted = false;
	if (ui.micMute_checkBox->checkState() == Qt::Checked) {
		// Mute
		isMuted = true;
	}

	pmainWindow->SetMuted(isMuted, disable);
	pmainWindow->update();
}

bool SettingPopUp::eventFilter(QObject* obj, QEvent* event)
{
	if (obj == ui.mic_Slider) {
		if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick || event->type() == QEvent::MouseMove) {
			if (ui.mic_Slider->isEnabled()) {
				QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
				int val = ui.mic_Slider->minimum() + (ui.mic_Slider->maximum() - ui.mic_Slider->minimum()) * (static_cast<float>(mouseEvent->x()) / static_cast<float>(ui.mic_Slider->width()));
				m_lastMicVolume = val;
				ui.mic_Slider->setValue(val);
				event->ignore();
				return true;
			}
		}
	}
	else {
		for (int i = 0; i < m_checkVec.size(); i++) {
			if (obj == m_checkVec[i]) {
				if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) {
					if (ui.hide_info_checkBox->isChecked()) {
						MaskingManager::Instance()->SetVisible(m_maskingAreaVec[i], !(m_checkVec[i]->isChecked()));
						QCheckBox* _check = static_cast<QCheckBox*>(obj);
						if (_check->isChecked()) {
							_check->setProperty("Style", "1");
						}
						else {
							_check->setProperty("Style", "0");
						}
						_check->style()->unpolish(_check);
						_check->style()->polish(_check);
						LocalSettings::SetGameMaskSettings(m_game_code, m_checkVec.size(), i, !_check->isChecked());
					}
					break;
				}
			}
		}
	}

	return QFramelessPopup::eventFilter(obj, event);
}

int SettingPopUp::getSettingPopUpWidth() 
{ 
	return SETTINGPOPUP_WIDTH; 
}

int SettingPopUp::getSettingPopUpHeight()
{
	int newh = SETTINGPOPUP_HEIGHT - SETTINGPOPUP_CHECKBOX_MAXIMUM_HEIGHT + 20 + (m_maskingAreaVec.size() - 1) * SETTINGPOPUP_CHECKBOX_Y_MARGIN;
	return newh;
}

void SettingPopUp::setCheckBoxLabelVec(std::vector<QWidget*>& _checkVector)
{
	for (int i = 0; i < _checkVector.size(); i++) {
		QCheckBox* pcb = static_cast<QCheckBox*>(_checkVector[i]);
		m_checkVec[i]->setChecked(pcb->isChecked());
		if(pcb->isChecked())
		{
			m_checkVec[i]->setProperty("Style", "0");
		}
		else
		{
			m_checkVec[i]->setProperty("Style", "1");
		}
		m_checkVec[i]->style()->unpolish(m_checkVec[i]);
		m_checkVec[i]->style()->polish(m_checkVec[i]);
		MaskingManager::Instance()->SetVisible(m_maskingAreaVec[i], (m_checkVec[i]->isChecked()));
	}
}

bool SettingPopUp::event(QEvent *event)
{
	if (event->type() == QEvent::WindowDeactivate) {
		//hide();
	}

	return QWidget::event(event);
}

void SettingPopUp::on_hide_info_checkBox_clicked(bool _bShowMask)
{
	for (int i = 0; i < m_maskingAreaVec.size(); i++) {
		if (_bShowMask) {
			MaskingManager::Instance()->SetVisible(m_maskingAreaVec[i], (m_checkVec[i]->isChecked()));
		}
		else {
			MaskingManager::Instance()->SetVisible(m_maskingAreaVec[i], false);
		}
		m_checkVec[i]->setDisabled(!_bShowMask);
	}

	LocalSettings::SetEnableGameMask(_bShowMask);
}

void SettingPopUp::SetMicInputLevel(unsigned int level)
{
	emit SetSliderMicLevel(level);
}

void SettingPopUp::SaveLocalMicInfo()
{
	LocalSettings::SetMicMute(ui.micMute_checkBox->isChecked());
	LocalSettings::SetMicVolume(float(ui.mic_Slider->value()));
	LocalSettings::SetSelectedMicDeviceID(QString::fromStdString(m_currSelectedMicDevice.id));
}

void SettingPopUp::hide()
{
	SaveLocalMicInfo();
	QFramelessPopup::hide();
}

void SettingPopUp::on_opensource_btn_clicked()
{
	if (m_dimWidget)
	{
		delete m_dimWidget;
		m_dimWidget = nullptr;
	}

	m_dimWidget = new dimmingWidget(m_parent);
	m_dimWidget->setCurrentGeo(m_parent);
	m_dimWidget->SetEnableResize(true);
	
	if (m_popenLicensePopup == nullptr) {
		m_popenLicensePopup = new OpenLicensePopup(m_dimWidget);
		m_popenLicensePopup->Init();
	}

	m_dimWidget->SetChild(m_popenLicensePopup);
	m_dimWidget->show();

	m_popenLicensePopup->Show();
}

void SettingPopUp::SetQcomboBorder(QColor color)
{
	m_qcomboBorder = color;

	if (m_pMicComboItemdel) {
		m_pMicComboItemdel->SetQcomboBorder(m_qcomboBorder);
	}
}

void SettingPopUp::SetQcomboBackground(QColor color)
{
	m_qcomboBackground = color;

	if (m_pMicComboItemdel) {
		m_pMicComboItemdel->SetQcomboBackground(m_qcomboBackground);
	}
}

void SettingPopUp::SetHighLightedText(QColor color)
{
	m_highLightedText = color;

	if (m_pMicComboItemdel) {
		m_pMicComboItemdel->SetHighLightedText(m_highLightedText);
	}
}

void SettingPopUp::SetNonHighLightedText(QColor color)
{
	m_nonHighLightedTextColor = color;

	if (m_pMicComboItemdel) {
		m_pMicComboItemdel->SetNonHighLightedText(m_nonHighLightedTextColor);
	}
}
