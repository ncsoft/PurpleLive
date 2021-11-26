#pragma once

#include "Singleton.h"
#include "VideoChatApp.h"
#include "easylogging++.h"
#include "MenuWidgets.h"
#include <thread>

class MenuControl : public CommunityLiveCore::Singleton<MenuControl>
{
public:
	MenuControl();
	virtual ~MenuControl();

	void ShowMainMenu(QWidget* container, QPoint pos);
	void ShowMaskMenu(QWidget* container, QPoint pos);
	void ShowMicMenu(QWidget* container, QPoint pos);
	void ShowLicensePopup();
	void SetMicMagnitude(float magnitude);

protected:
	static void SetProperty(QWidget* object, const char *name, const QVariant &value);
	static void SetTransparentRoundedCorner(QWidget* widget);
	void AddShadowEffect(QWidget* widget);
	
	void SetEnableRenderMicMagnitude(bool enable);
	void InitMenu(QMenu* menu);
	void AddCustomSeparator(QWidget* container, QMenu* menu);
	void AddLicenseMenu(QWidget* container, QMenu* main_menu);
	void AddMaskMenu(QWidget* container, QMenu* mask_menu);
	void AddMicMenu(QWidget* container, QMenu* mic_menu);
	void AddLanguageMenu(QWidget* container, QMenu* theme_menu);
	void AddThemeMenu(QWidget* container, QMenu* theme_menu);
	void AddTestMenu(QWidget* container, QMenu* test_menu);
	
	void AddMaskAllMenu(QWidget* container, QMenu* mask_menu, bool is_mask_enable);
	void AddMaskItemMenu(QWidget* container, QMenu* mask_menu, QString& mask_name, bool is_mask, bool is_mask_enable);
	void AddMicTitleMenu(QWidget* container, QMenu* mic_menu, QString& title);
	void AddMicItemMenu(QWidget* container, QMenu* mic_menu, int index, QString mic_name, bool is_default, bool is_selected);
	void AddMicVolumeMenu(QWidget* container, QMenu* mask_menu, bool is_enable);
	
	void SetMicIndex(int index);
	void SetEnableMask(bool enable);
	void ToggleMaskItem(int index, bool enable);

	QString GetQss(eThemeType type);
	QString GetQssThemeLight();

private:
	std::vector<QCheckBox*> m_mask_checkbox_list;
	std::vector<QCheckBox*> m_mic_checkbox_list;
	std::vector<AudioDeviceInfo> m_mic_list;

	CustomMenu* m_menu = nullptr;
	QCheckBox* m_mic_mute_checkbox = nullptr;
	VolumeSlider* m_mic_volume_slider = nullptr;
	float m_mic_magnitude = 0.0f;
	QTimer m_mic_magnitude_timer;
	std::thread m_mic_magnitude_thread;
	bool m_mic_magnitude_thread_stop = false;
};
