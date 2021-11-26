#include "MenuControl.h"
#include "MenuWidgets.h"
#include "VideoChatApp.h"
#include "MainWindow.h"
#include "LocalSettings.h"
#include "HiddenSettings.h"
#include "MaskingManager.h"
#include "AppUtils.h"
#include "AppConfig.h"
#include "AppString.h"
#include "easylogging++.h"
#include "VideoChatMessageBox.h"
#include "webview/WebviewControl.h"
#include "OpenLicensePopup.h"
#include "MuteCheckBox.h"

#include <QMenu>
#include <QWidgetAction>
#include <QToolButton>
#include <QGraphicsDropShadowEffect>
#include <QClipboard>

MenuControl::MenuControl()
{

}

MenuControl::~MenuControl()
{
	elogd("[MenuControl::~MenuControl] Singleton");
}

void MenuControl::ShowMainMenu(QWidget* container, QPoint pos)
{
	CustomMenu menu_object{container};
	m_menu = &menu_object;
	m_menu->setStyleSheet(App()->styleSheet()); // for qproperty

	// 1. main menu
	CustomMenu* main_menu = m_menu;
	main_menu->setObjectName(QString::fromUtf8("main_menu_object"));
	InitMenu(main_menu);

	// 2. mask menu
	ChildMenu* mask_menu = new ChildMenu(AppString::GetQString("main_menu/main_menu_mask"), main_menu);
	mask_menu->setObjectName(QString::fromUtf8("main_menu_object"));
	AddMaskMenu(container, mask_menu);
	InitMenu(mask_menu);
	main_menu->addMenu(mask_menu);

	// 3. audio menu
	ChildMenu* mic_menu = new ChildMenu(AppString::GetQString("main_menu/main_menu_mic"), main_menu);
	mic_menu->setObjectName(QString::fromUtf8("main_menu_object"));
    AddMicMenu(container, mic_menu);
	InitMenu(mic_menu);
	main_menu->addMenu(mic_menu);

	// 4. license menu
	AddLicenseMenu(container, main_menu);

	SetEnableRenderMicMagnitude(true);
	QAction *res = m_menu->exec(pos);
	SetEnableRenderMicMagnitude(false);
	m_menu = nullptr;
}

void MenuControl::ShowMaskMenu(QWidget* container, QPoint pos)
{
	CustomMenu menu;
	menu.setStyleSheet(GetQss(App()->GetTheme()));
    AddMicMenu(container, &menu);
	InitMenu(&menu);	
	QAction *res = menu.exec(QCursor::pos());
}

void MenuControl::ShowMicMenu(QWidget* container, QPoint pos)
{
	CustomMenu menu;
	menu.setStyleSheet(GetQss(App()->GetTheme()));
	AddMicMenu(container, &menu);
	InitMenu(&menu);
    QAction *res = menu.exec(QCursor::pos());
}

void MenuControl::ShowLicensePopup()
{
	auto popup = new OpenLicensePopup(MainWindow::Get());
	popup->Init();
	popup->Show();
}

void MenuControl::SetMicMagnitude(float magnitude)
{
	m_mic_magnitude = magnitude;
}

void MenuControl::SetProperty(QWidget* widget, const char *name, const QVariant &value)
{
	widget->setProperty(name, value);
	widget->style()->unpolish(widget);
	widget->style()->polish(widget);
}

void MenuControl::SetTransparentRoundedCorner(QWidget* widget)
{
	Qt::WindowFlags flags = Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint;
	widget->setWindowFlags(widget->windowType() | flags);
	widget->setAttribute(Qt::WA_TranslucentBackground);
}

void MenuControl::AddShadowEffect(QWidget* widget)
{
	QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect(widget); 
	// QT bug : QT error message when use setBlurRadius (https://bugreports.qt.io/browse/QTBUG-58602)
	// QT error message "UpdateLayeredWindowIndirect failed for ptDst=(489, 551), size=(259x210), dirty=(279x230 -10, 0) ..."
	effect->setBlurRadius(10); 
	effect->setColor(m_menu->GetShadowColor());
	effect->setOffset(QPointF(0,10));
	widget->setGraphicsEffect(effect);
}

void MenuControl::InitMenu(QMenu* menu)
{
	SetTransparentRoundedCorner(menu);
	AddShadowEffect(menu);
}

// ContextMenu에서 타이머 적용 시 비주기적으로 쓰레드 오류 발생
// QObject::startTimer: Timers can only be used with threads started with QThread
// QObject::killTimer: Timers cannot be stopped from another thread
#define USE_RENDER_MAGNITUDE_THREAD	1
void MenuControl::SetEnableRenderMicMagnitude(bool enable)
{
	//elogd("[MenuControl::SetEnableRenderMicMagnitude] enable(%d) m_menu(0x%p)", enable, m_menu);

#if USE_RENDER_MAGNITUDE_THREAD
	if (enable)
	{
		m_mic_magnitude_thread_stop = false;
		m_mic_magnitude_thread = std::thread([this]()
		{
			while (!m_mic_magnitude_thread_stop)
			{
				if (m_menu && m_mic_volume_slider)
				{
					//elogd("[MenuControl::SetEnableRenderMicMagnitude::m_mic_magnitude_thread] m_mic_magnitude(%f)", m_mic_magnitude);
					if (m_menu && m_mic_volume_slider) {
						AppUtils::DispatchToMainThread([this]() {
							m_mic_volume_slider->setMagnitude(m_mic_magnitude);
						});
					}
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		});
	}
	else
	{
		m_mic_magnitude_thread_stop = true;
		if (m_mic_magnitude_thread.joinable())
			m_mic_magnitude_thread.join();
	}
#else
	if (enable) 
	{
		if (m_menu && m_mic_volume_slider)
		{
			m_menu->connect(&m_mic_magnitude_timer, &QTimer::timeout, m_menu, [this]() {
				//elogd("[MenuControl::SetEnableRenderMicMagnitude::m_mic_magnitude_timer] m_mic_magnitude(%d)", m_mic_magnitude);
				if (m_menu && m_mic_volume_slider) {
					m_mic_volume_slider->setMagnitude(m_mic_magnitude);
				}
			});
			m_mic_magnitude_timer.setInterval(10);
			m_mic_magnitude_timer.start();
		}
	}
	else 
	{
		m_mic_magnitude_timer.stop();
	}
#endif
}

void MenuControl::AddCustomSeparator(QWidget* container, QMenu* menu)
{
	constexpr int depth = 0;
	constexpr int margin_left = 8+(8*depth);
	auto action = new QWidgetAction(container);
	auto widget = new MenuWidget(container);
	auto layout = new QHBoxLayout();
	widget->setObjectName(QString::fromUtf8("separator_menu_item_widget"));
	widget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
	widget->setFixedHeight(19);
	widget->setLayout(layout);
	action->setDefaultWidget(widget);
	menu->addAction(action);
}

void MenuControl::AddLicenseMenu(QWidget* container, QMenu* main_menu)
{
	constexpr int depth = 0;
	constexpr int margin_left = 8+(8*depth);
	auto action = new QWidgetAction(container);
	auto widget = new MenuWidget(container);
	auto layout = new QHBoxLayout();
	
	widget->setObjectName(QString::fromUtf8("license_menu_item_widget"));
	widget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

	MenuButton* menu_button_title = new MenuButton(AppString::GetQString("main_menu/main_menu_license"), widget);
	menu_button_title->setObjectName(QString::fromUtf8("license_menu_button_title"));
	
	MenuButton* menu_button_image = new MenuButtonLicenseIcon(widget);
	menu_button_image->setObjectName(QString::fromUtf8("license_menu_button_image"));
    
	layout->setSpacing(0);
	layout->setMargin(0);
	layout->addSpacing(margin_left);
	layout->setAlignment(Qt::AlignLeft);
	layout->addWidget(menu_button_title);
	layout->addWidget(menu_button_image);
	layout->addStretch();

	widget->setLayout(layout);
	action->setDefaultWidget(widget);
	main_menu->addAction(action);

	auto click_handler = [container, widget, menu_button_title, menu_button_image]() {
		MenuControl::Instance()->ShowLicensePopup();
	};
	auto hoverd_handler = [action, widget, menu_button_title, menu_button_image]() {
		if (widget->property("state")=="hover")
			return;
		SetProperty(widget, "state", "hover");
		SetProperty(menu_button_title, "state", "hover");
		SetProperty(menu_button_image, "state", "hover");
	};
	auto leaved_handler = [widget, menu_button_title, menu_button_image]() {
		if (widget->property("state")=="normal")
			return;
		SetProperty(widget, "state", "normal");
		SetProperty(menu_button_title, "state", "normal");
		SetProperty(menu_button_image, "state", "normal");
	};

	container->connect(widget, &MenuWidget::OnMousePress, container, click_handler);
	container->connect(menu_button_title, &MenuButton::clicked, container, click_handler);
	container->connect(menu_button_image, &MenuButton::clicked, container, click_handler);

	container->connect(widget, &MenuWidget::OnMouseHover, container, hoverd_handler);
	container->connect(menu_button_title, &MenuButton::OnMouseHover, container, hoverd_handler);
	container->connect(menu_button_image, &MenuButton::OnMouseHover, container, hoverd_handler);
	
	container->connect(widget, &MenuWidget::OnMouseLeave, container, leaved_handler);
	//container->connect(menu_button_title, &MenuButton::MouseLeaved, container, leaved_handler);
	//container->connect(menu_button_image, &MenuButton::MouseLeaved, container, leaved_handler);

	// bugfix : QWidgetAction highlight bug (https://stackoverflow.com/questions/55086498/highlighting-custom-qwidgetaction-on-hover)
	container->connect(main_menu, &QMenu::hovered, container, [widget, hoverd_handler, leaved_handler]() {
		QRect rect{widget->mapToGlobal({0,0}), widget->size()};
		if (rect.contains(QCursor::pos())) {
			hoverd_handler();
		} else {
			leaved_handler();
		}
	});
}

void MenuControl::AddMaskMenu(QWidget* container, QMenu* mask_menu)
{
	std::vector<std::wstring> mask_list;
	MaskingManager::Instance()->GetMaskNameList(mask_list);
	
	bool is_enable_game_mask = LocalSettings::GetEnableGameMask(LocalSettings::DEFAULT_ENABLE_GAME_MASK);
	QBitArray game_mask_settings = LocalSettings::GetGameMaskSettings(App()->GetOriginGameCode(), QBitArray(mask_list.size()));

	AddMaskAllMenu(container, mask_menu, is_enable_game_mask);	
	
	m_mask_checkbox_list.clear();
	for (int i=0; i<mask_list.size(); i++)
	{
		AddMaskItemMenu(container, mask_menu, QString::fromStdWString(mask_list[i]), game_mask_settings.at(i), is_enable_game_mask);
	}
}

void MenuControl::AddMicMenu(QWidget* container, QMenu* mic_menu)
{
	// 1. mic menu title
	AddMicTitleMenu(container, mic_menu, AppString::GetQString("main_menu/main_menu_mic_select"));

	bool is_default_mic = false;
	bool is_selected_not_use_mic = false;
	std::string selected_mic_id = LocalSettings::GetSelectedMicDeviceID("").toStdString();
	if (selected_mic_id.empty()) {
		is_default_mic = true;
	}
	else if (selected_mic_id == AppConfig::not_use_mic_device) {
		is_selected_not_use_mic = true;
	}

	// 2. mic device list
	int mic_index = 0;
	m_mic_checkbox_list.clear();
	m_mic_list = NCAudioMixer::Instance()->GetDeviceInfoList(1);
	for (int i=0; i<m_mic_list.size(); i++)
	{
		bool is_selected = (selected_mic_id==m_mic_list[i].id);
		if (is_default_mic && m_mic_list[i].is_default)
		{
			is_selected = true;
			mic_index = i;
		}
		AddMicItemMenu(container, mic_menu, i, StringUtils::Local8bitToUnicode(m_mic_list[i].name.c_str()), m_mic_list[i].is_default, is_selected);
	}

	// 3. mic unuse item
	mic_index = (is_selected_not_use_mic) ? m_mic_list.size() : mic_index;
	AddMicItemMenu(container, mic_menu, m_mic_list.size(), AppString::GetQString("main_menu/main_menu_unuse_mic"), false, is_selected_not_use_mic);

	// 4. volume control title 
	AddCustomSeparator(container, mic_menu);
	AddMicTitleMenu(container, mic_menu, AppString::GetQString("main_menu/main_menu_volume_control"));

	// 5. volume slider
	AddMicVolumeMenu(container, mic_menu, !is_selected_not_use_mic);
}

void MenuControl::AddLanguageMenu(QWidget* container, QMenu* lang_menu)
{
	QAction* act_lang_ko_KR = lang_menu->addAction(LanguageCode::ko_KR);
	QAction* act_lang_en_US = lang_menu->addAction(LanguageCode::en_US);
	QAction* act_lang_zh_TW = lang_menu->addAction(LanguageCode::zh_TW);
	QAction* act_lang_zh_CN = lang_menu->addAction(LanguageCode::zh_CN);
	QAction* act_lang_de_DE = lang_menu->addAction(LanguageCode::de_DE);
	QAction* act_lang_es_ES = lang_menu->addAction(LanguageCode::es_ES);
	QAction* act_lang_fr_FR = lang_menu->addAction(LanguageCode::fr_FR);
	QAction* act_lang_id_ID = lang_menu->addAction(LanguageCode::id_ID);
	QAction* act_lang_ja_JP = lang_menu->addAction(LanguageCode::ja_JP);
	QAction* act_lang_pl_PL = lang_menu->addAction(LanguageCode::pl_PL);
	QAction* act_lang_ru_RU = lang_menu->addAction(LanguageCode::ru_RU);
	QAction* act_lang_th_TH = lang_menu->addAction(LanguageCode::th_TH);
	QAction* act_lang_ar_SA = lang_menu->addAction(LanguageCode::ar_SA);
	QAction* act_lang_vi_VN = lang_menu->addAction(LanguageCode::vi_VN);
	QAction* act_lang_tr_TR = lang_menu->addAction(LanguageCode::tr_TR);

	act_lang_ko_KR->setCheckable(true);
	act_lang_en_US->setCheckable(true);
	act_lang_zh_TW->setCheckable(true);
	act_lang_zh_CN->setCheckable(true);
	act_lang_de_DE->setCheckable(true);
	act_lang_es_ES->setCheckable(true);
	act_lang_fr_FR->setCheckable(true);
	act_lang_id_ID->setCheckable(true);
	act_lang_ja_JP->setCheckable(true);
	act_lang_pl_PL->setCheckable(true);
	act_lang_ru_RU->setCheckable(true);
	act_lang_th_TH->setCheckable(true);
	act_lang_ar_SA->setCheckable(true);
	act_lang_vi_VN->setCheckable(true);
	act_lang_tr_TR->setCheckable(true);

	act_lang_ko_KR->setChecked(false);
	act_lang_en_US->setChecked(false);
	act_lang_zh_TW->setChecked(false);
	act_lang_zh_CN->setChecked(false);
	act_lang_de_DE->setChecked(false);
	act_lang_es_ES->setChecked(false);
	act_lang_fr_FR->setChecked(false);
	act_lang_id_ID->setChecked(false);
	act_lang_ja_JP->setChecked(false);
	act_lang_pl_PL->setChecked(false);
	act_lang_ru_RU->setChecked(false);
	act_lang_th_TH->setChecked(false);
	act_lang_ar_SA->setChecked(false);
	act_lang_vi_VN->setChecked(false);
	act_lang_tr_TR->setChecked(false);

	QString language = App()->GetLocalLanguage();
	if (language == LanguageCode::ko_KR)
		act_lang_ko_KR->setChecked(true);
	else if (language == LanguageCode::en_US)
		act_lang_en_US->setChecked(true);
	else if (language == LanguageCode::zh_TW)
		act_lang_zh_TW->setChecked(true);
	else if (language == LanguageCode::zh_CN)
		act_lang_zh_CN->setChecked(true);
	else if (language == LanguageCode::de_DE)
		act_lang_de_DE->setChecked(true);
	else if (language == LanguageCode::es_ES)
		act_lang_es_ES->setChecked(true);
	else if (language == LanguageCode::fr_FR)
		act_lang_fr_FR->setChecked(true);
	else if (language == LanguageCode::id_ID)
		act_lang_id_ID->setChecked(true);
	else if (language == LanguageCode::ja_JP)
		act_lang_ja_JP->setChecked(true);
	else if (language == LanguageCode::pl_PL)
		act_lang_pl_PL->setChecked(true);
	else if (language == LanguageCode::ru_RU)
		act_lang_ru_RU->setChecked(true);
	else if (language == LanguageCode::th_TH)
		act_lang_th_TH->setChecked(true);
	else if (language == LanguageCode::ar_SA)
		act_lang_ar_SA->setChecked(true);
	else if (language == LanguageCode::vi_VN)
		act_lang_vi_VN->setChecked(true);
	else if (language == LanguageCode::tr_TR)
		act_lang_tr_TR->setChecked(true);
	
	container->connect(act_lang_ko_KR, &QAction::triggered, container, [](bool checked) {
		App()->SetLocalLanguage(LanguageCode::ko_KR);
	});
	container->connect(act_lang_en_US, &QAction::triggered, container, [](bool checked) {
		App()->SetLocalLanguage(LanguageCode::en_US);
	});
	container->connect(act_lang_zh_TW, &QAction::triggered, container, [](bool checked) {
		App()->SetLocalLanguage(LanguageCode::zh_TW);
		});
	container->connect(act_lang_zh_CN, &QAction::triggered, container, [](bool checked) {
		App()->SetLocalLanguage(LanguageCode::zh_CN);
		});
	container->connect(act_lang_de_DE, &QAction::triggered, container, [](bool checked) {
		App()->SetLocalLanguage(LanguageCode::de_DE);
		});
	container->connect(act_lang_es_ES, &QAction::triggered, container, [](bool checked) {
		App()->SetLocalLanguage(LanguageCode::es_ES);
		});
	container->connect(act_lang_fr_FR, &QAction::triggered, container, [](bool checked) {
		App()->SetLocalLanguage(LanguageCode::fr_FR);
		});
	container->connect(act_lang_id_ID, &QAction::triggered, container, [](bool checked) {
		App()->SetLocalLanguage(LanguageCode::id_ID);
		});
	container->connect(act_lang_ja_JP, &QAction::triggered, container, [](bool checked) {
		App()->SetLocalLanguage(LanguageCode::ja_JP);
		});
	container->connect(act_lang_pl_PL, &QAction::triggered, container, [](bool checked) {
		App()->SetLocalLanguage(LanguageCode::pl_PL);
		});
	container->connect(act_lang_ru_RU, &QAction::triggered, container, [](bool checked) {
		App()->SetLocalLanguage(LanguageCode::ru_RU);
		});
	container->connect(act_lang_th_TH, &QAction::triggered, container, [](bool checked) {
		App()->SetLocalLanguage(LanguageCode::th_TH);
		});
	container->connect(act_lang_ar_SA, &QAction::triggered, container, [](bool checked) {
		App()->SetLocalLanguage(LanguageCode::ar_SA);
		});
	container->connect(act_lang_vi_VN, &QAction::triggered, container, [](bool checked) {
		App()->SetLocalLanguage(LanguageCode::vi_VN);
	});
	container->connect(act_lang_tr_TR, &QAction::triggered, container, [](bool checked) {
		App()->SetLocalLanguage(LanguageCode::tr_TR);
	});
}

void MenuControl::AddThemeMenu(QWidget* container, QMenu* theme_menu)
{
	QAction* act_theme_light = theme_menu->addAction(AppString::GetQString("main_menu/main_menu_theme_light"));
	QAction* act_theme_purple = theme_menu->addAction(AppString::GetQString("main_menu/main_menu_theme_purple"));
	QAction* act_theme_dark = theme_menu->addAction(AppString::GetQString("main_menu/main_menu_theme_dark"));

	act_theme_light->setCheckable(true);
	act_theme_purple->setCheckable(true);
	act_theme_dark->setCheckable(true);

	switch (App()->GetTheme())
	{
		case eThemeType::eTT_Light:
			act_theme_light->setChecked(true);
			act_theme_purple->setChecked(false);
			act_theme_dark->setChecked(false);
			break;
		case eThemeType::eTT_Purple:
			act_theme_light->setChecked(false);
			act_theme_purple->setChecked(true);
			act_theme_dark->setChecked(false);
			break;
		case eThemeType::eTT_Dark:
			act_theme_light->setChecked(false);
			act_theme_purple->setChecked(false);
			act_theme_dark->setChecked(true);
			break;
		default:
			act_theme_light->setChecked(false);
			act_theme_purple->setChecked(false);
			act_theme_dark->setChecked(false);
	}

	container->connect(act_theme_light, &QAction::triggered, container, [](bool checked) {
		App()->SetTheme(eThemeType::eTT_Light);
	});
	container->connect(act_theme_purple, &QAction::triggered, container, [](bool checked) {
		App()->SetTheme(eThemeType::eTT_Purple);
	});
	container->connect(act_theme_dark, &QAction::triggered, container, [](bool checked) {
		App()->SetTheme(eThemeType::eTT_Dark);
	}); 
}

void MenuControl::AddTestMenu(QWidget* container, QMenu* test_menu)
{
	ChildMenu* hidden_menu = new ChildMenu(StringUtils::Local8bitToUnicode("HiddenSettings"), test_menu);
	hidden_menu->setObjectName(QString::fromUtf8("main_menu_object"));
	InitMenu(hidden_menu), test_menu->addMenu(hidden_menu);

	QAction* act_app_debug = hidden_menu->addAction("App Debug");
	QAction* act_web_debug = hidden_menu->addAction("Web Debug");
	act_app_debug->setCheckable(true);
	act_web_debug->setCheckable(true);
	act_app_debug->setChecked(HiddenSettings::Instance()->GetEnableAppDebug());
	act_web_debug->setChecked(HiddenSettings::Instance()->GetEnableWebDebug());

	container->connect(act_app_debug, &QAction::triggered, container, []() {
		bool enable = HiddenSettings::Instance()->GetEnableAppDebug();
		HiddenSettings::Instance()->SetEnableAppDebug(!enable);
	});
	container->connect(act_web_debug, &QAction::triggered, container, []() {
		bool enable = HiddenSettings::Instance()->GetEnableWebDebug();
		HiddenSettings::Instance()->SetEnableWebDebug(!enable);
	});
	QAction* act_del_hidden_settings = hidden_menu->addAction("remove HiddenSettings");
	container->connect(act_del_hidden_settings, &QAction::triggered, container, []() {
		HiddenSettings::Instance()->SetEnableAppDebug(false);
		HiddenSettings::Instance()->SetEnableWebDebug(false);
		HiddenSettings::Instance()->RemoveFile();
		VideoChatMessageBox::Instance()->information(StringUtils::Local8bitToUnicode("remove HiddenSettings").toStdString());
	});
}

void AddFontItemMenu(QWidget* container, QMenu* font_menu, QString text, QString font_name, int size, int weight)
{
	constexpr int depth = 0;
	constexpr int margin_left = 8+(8*depth);
	constexpr int margin_right = 8;
	auto action = new QWidgetAction(container);
	auto widget = new MenuWidget(container);
	auto layout = new QHBoxLayout();
	widget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

	QLabel* menu_button_title = new QLabel(text, widget);
	
	layout->setSpacing(0);
	layout->setMargin(0);

	layout->addSpacing(margin_left);
	layout->addWidget(menu_button_title);
	layout->addSpacing(margin_right);

	widget->setLayout(layout);
	action->setDefaultWidget( widget );
	font_menu->addAction( action );

	menu_button_title->setStyleSheet(QStringLiteral(
	R"DELIM(

QLabel { 
	font-family: '%1'; font-weight: %3; 
	font-size: %2px;
	text-align: left;
	background-color: transparent; 
	color: green;
}

	)DELIM").arg(font_name, QVariant(size).toString(), QVariant(weight).toString()));
}

void MenuControl::AddMaskAllMenu(QWidget* container, QMenu* mask_menu, bool is_mask_enable)
{
	constexpr int depth = 0;
	constexpr int margin_left = 8+(8*depth);
	constexpr int margin_right = 8;
	constexpr int space = 25;
	auto action = new QWidgetAction(container);
	auto widget = new MenuWidget(container);
	auto layout = new QHBoxLayout();
	
	widget->setObjectName(QString::fromUtf8("maskall_menu_item_widget"));
	widget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

	MenuButton* menu_button_title = new MenuButton(AppString::GetQString("main_menu/main_menu_mask_all"), widget);
	menu_button_title->setObjectName(QString::fromUtf8("maskall_menu_button_title"));
	
	MenuCheckBox* menu_checkbox_mask_enable = new MenuCheckBox(widget);
	menu_checkbox_mask_enable->setObjectName(QString::fromUtf8("maskall_menu_checkbox_mask_enable"));
    menu_checkbox_mask_enable->setFixedSize(34, 32);
	if (is_mask_enable)
		menu_checkbox_mask_enable->toggle();

	layout->setSpacing(0);
	layout->setMargin(0);

	layout->addSpacing(margin_left);
	layout->addWidget(menu_button_title);
	layout->addSpacing(space);
	layout->addWidget(menu_checkbox_mask_enable);
	layout->addSpacing(margin_right);

	widget->setLayout(layout);
	action->setDefaultWidget( widget );
	mask_menu->addAction( action );

	auto hoverd_handler = [widget, menu_button_title, menu_checkbox_mask_enable]() {
		if (widget->property("state")=="hover")
			return;
		SetProperty(widget, "state", "hover");
		SetProperty(menu_button_title, "state", "hover");
		SetProperty(menu_checkbox_mask_enable, "state", "hover");
	};
	auto leaved_handler = [widget, menu_button_title, menu_checkbox_mask_enable]() {
		if (widget->property("state")=="normal")
			return;
		SetProperty(widget, "state", "normal");
		SetProperty(menu_button_title, "state", "normal");
		SetProperty(menu_checkbox_mask_enable, "state", "normal");
	};
	auto click_handler = [menu_checkbox_mask_enable]() {
		menu_checkbox_mask_enable->toggle();
	};
	auto toggle_handler = [this](bool checked) {
		SetEnableMask(checked);
	};

	container->connect(widget, &MenuWidget::OnMouseHover, container, hoverd_handler);
	container->connect(menu_button_title, &MenuButton::OnMouseHover, container, hoverd_handler);
	container->connect(menu_checkbox_mask_enable, &MenuCheckBox::OnMouseHover, container, hoverd_handler);
	
	container->connect(widget, &MenuWidget::OnShow, container, leaved_handler);
	container->connect(widget, &MenuWidget::OnMouseLeave, container, leaved_handler);
	//container->connect(menu_button_title, &MenuButton::MouseLeaved, container, leaved_handler);
	//container->connect(menu_checkbox_mask_enable, &MenuCheckBox::MouseLeaved, container, leaved_handler);

	container->connect(widget, &MenuWidget::OnMousePress, container, click_handler);
	container->connect(menu_button_title, &MenuButton::clicked, container, click_handler);
	container->connect(menu_checkbox_mask_enable, &QCheckBox::toggled, container, toggle_handler);
}

void MenuControl::AddMaskItemMenu(QWidget* container, QMenu* mask_menu, QString& mask_name, bool is_mask, bool is_mask_enable)
{
	constexpr int depth = 1;
	constexpr int margin_left = 8+(8*depth);
	constexpr int margin_right = 8;
	constexpr int space = 25;
	auto action = new QWidgetAction(container);
	auto widget = new MenuWidget(container);
	auto layout = new QHBoxLayout();
	
	widget->setObjectName(QString::fromUtf8("mask_menu_item_widget"));
	widget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

	MenuButton* menu_item_button = new MenuButton(mask_name);
	menu_item_button->setObjectName(QString::fromUtf8("mask_menu_item_button"));
		
	MenuCheckBox* menu_item_checkbox = new MenuCheckBox(widget);
	m_mask_checkbox_list.push_back(menu_item_checkbox);
    menu_item_checkbox->setObjectName(QString::fromUtf8("mask_menu_item_checkbox"));
    menu_item_checkbox->setFixedSize(24, 24);
	menu_item_checkbox->setDisabled(!is_mask_enable);
	menu_item_checkbox->setProperty("enable_mask", is_mask_enable ? true : false);
	if (is_mask)
		menu_item_checkbox->toggle();

	layout->setSpacing(0);
	layout->setMargin(0);
	layout->addSpacing(margin_left);
	layout->addWidget(menu_item_button);
	layout->addSpacing(space);
	layout->addWidget(menu_item_checkbox);
	layout->addSpacing(margin_right);

	widget->setLayout(layout);
	action->setDefaultWidget(widget);
	mask_menu->addAction(action);

	auto hoverd_handler = [widget, menu_item_button, menu_item_checkbox]() {
		if (widget->property("state")=="hover")
			return;
		if (!menu_item_checkbox->isEnabled())
			return;
		SetProperty(widget, "state", "hover");
		SetProperty(menu_item_button, "state", "hover");
		SetProperty(menu_item_checkbox, "state", "hover");
	};
	auto leaved_handler = [widget, menu_item_button, menu_item_checkbox]() {
		if (widget->property("state")=="normal")
			return;
		SetProperty(widget, "state", "normal");
		SetProperty(menu_item_button, "state", "normal");
		SetProperty(menu_item_checkbox, "state", "normal");
	};
	auto click_handler = [menu_item_checkbox]() {
		if (menu_item_checkbox->isEnabled())
			menu_item_checkbox->toggle();
	};
	auto toggle_handler = [this, index=m_mask_checkbox_list.size()-1](bool checked) {
		ToggleMaskItem(index, checked);
	};

	container->connect(widget, &MenuWidget::OnMouseHover, container, hoverd_handler);
	container->connect(menu_item_button, &MenuButton::OnMouseHover, container, hoverd_handler);
	container->connect(menu_item_checkbox, &MenuCheckBox::OnMouseHover, container, hoverd_handler);
	
	container->connect(widget, &MenuWidget::OnShow, container, leaved_handler);
	container->connect(widget, &MenuWidget::OnMouseLeave, container, leaved_handler);
	//container->connect(menu_button_title, &MenuButton::MouseLeaved, container, leaved_handler);
	//container->connect(menu_checkbox_mask_enable, &MenuCheckBox::MouseLeaved, container, leaved_handler);

	container->connect(widget, &MenuWidget::OnMousePress, container, click_handler);
	container->connect(menu_item_button, &MenuButton::clicked, container, click_handler);
	container->connect(menu_item_checkbox, &QCheckBox::toggled, container, toggle_handler);
}

void MenuControl::AddMicTitleMenu(QWidget* container, QMenu* mic_menu, QString& title)
{
	constexpr int depth = 0;
	constexpr int margin_left = 8+(8*depth);
	constexpr int margin_right = 8;
	constexpr int space = 25;
	auto action = new QWidgetAction(container);
	auto widget = new MenuWidget(container);
	auto layout = new QHBoxLayout();
	
	widget->setObjectName(QString::fromUtf8("mic_title_menu_item_widget"));
	widget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

	QPushButton* menu_button_title = new QPushButton(title);
	menu_button_title->setObjectName(QString::fromUtf8("mic_title_menu_button_title"));
	
	layout->setSpacing(0);
	layout->setMargin(0);
	layout->addSpacing(margin_left);
	layout->addWidget(menu_button_title);
	layout->addSpacing(space);
	layout->addSpacing(margin_right);

	widget->setLayout(layout);
	action->setDefaultWidget( widget );
	mic_menu->addAction( action );
}

void MenuControl::AddMicItemMenu(QWidget* container, QMenu* mic_menu, int index, QString mic_name, bool is_default, bool is_selected)
{
	//elogd("[MenuControl::AddMicItemMenu::toggled] index(%d) is_default(%d) is_selected(%d)", index, is_default, is_selected);
	constexpr int depth = 1;
	constexpr int margin_left = 8+(8*depth);
	constexpr int margin_right = 8;
	constexpr int space = 25;
	auto action = new QWidgetAction(container);
	auto widget = new MenuWidget(container);
	auto layout = new QHBoxLayout();
	
	widget->setObjectName(QString::fromUtf8("mic_device_menu_item_widget"));
	widget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

	if (is_default)
		mic_name.sprintf(AppString::GetQString("main_menu/main_menu_default_device_format").toStdString().c_str(), mic_name.toStdString().c_str());
	MenuButton* menu_item_button = new MenuButton(mic_name);
	menu_item_button->setObjectName(QString::fromUtf8("mic_device_menu_item_button"));
		
	MenuCheckBox* menu_item_checkbox = new MenuCheckBox(widget);
	m_mic_checkbox_list.push_back(menu_item_checkbox);
	menu_item_checkbox->setObjectName(QString::fromUtf8("mic_device_menu_item_checkbox"));
    menu_item_checkbox->setFixedSize(24, 24);
	menu_item_checkbox->setChecked(is_selected);
	
	layout->setSpacing(0);
	layout->setMargin(0);
	layout->addSpacing(margin_left);
	layout->addWidget(menu_item_button);
	layout->addSpacing(space);
	layout->addWidget(menu_item_checkbox);
	layout->addSpacing(margin_right);

	widget->setLayout(layout);
	action->setDefaultWidget( widget );
	mic_menu->addAction( action );

	auto hoverd_handler = [widget, menu_item_button, menu_item_checkbox]() {
		if (widget->property("state")=="hover")
			return;
		SetProperty(widget, "state", "hover");
		SetProperty(menu_item_button, "state", "hover");
		SetProperty(menu_item_checkbox, "state", "hover");
	};
	auto leaved_handler = [widget, menu_item_button, menu_item_checkbox]() {
		if (widget->property("state")=="normal")
			return;
		SetProperty(widget, "state", "normal");
		SetProperty(menu_item_button, "state", "normal");
		SetProperty(menu_item_checkbox, "state", "normal");
	};
	auto click_handler = [this, index]() {
		SetMicIndex(index);
	};

	container->connect(widget, &MenuWidget::OnMouseHover, container, hoverd_handler);
	container->connect(menu_item_button, &MenuButton::OnMouseHover, container, hoverd_handler);
	container->connect(menu_item_checkbox, &MenuCheckBox::OnMouseHover, container, hoverd_handler);
	
	container->connect(widget, &MenuWidget::OnShow, container, leaved_handler);
	container->connect(widget, &MenuWidget::OnMouseLeave, container, leaved_handler);
	//container->connect(menu_button_title, &MenuButton::MouseLeaved, container, leaved_handler);
	//container->connect(menu_checkbox_mask_enable, &MenuCheckBox::MouseLeaved, container, leaved_handler);

	container->connect(widget, &MenuWidget::OnMousePress, container, click_handler);
	container->connect(menu_item_button, &MenuButton::clicked, container, click_handler);
	container->connect(menu_item_checkbox, &QCheckBox::clicked, container, click_handler);
}

void MenuControl::AddMicVolumeMenu(QWidget* container, QMenu* mic_menu, bool is_enable)
{
	constexpr int depth = 0;
	constexpr int margin_left = 8+(8*depth);
	constexpr int margin_right = 8;
	constexpr int space = 4;
	auto action = new QWidgetAction(container);
	auto widget = new MenuWidget(container);
	auto layout = new QHBoxLayout();
	
	widget->setObjectName(QString::fromUtf8("volume_menu_item_widget"));
	widget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

	MuteCheckBox* mic_mute_checkbox = new MuteCheckBox(widget);
	m_mic_mute_checkbox = mic_mute_checkbox;
	mic_mute_checkbox->setObjectName(QString::fromUtf8("volume_menu_item_checkbox"));
    mic_mute_checkbox->setFixedSize(24, 24);

	VolumeSlider *mic_volume_slider = new VolumeSlider(widget);
	m_mic_volume_slider = mic_volume_slider;
	mic_volume_slider->setObjectName(QString::fromUtf8("volume_menu_item_slider"));
	mic_volume_slider->setOrientation(Qt::Horizontal);

	layout->setSpacing(0);
	layout->setMargin(0);
	layout->addSpacing(margin_left);
	layout->addWidget(mic_mute_checkbox);
	layout->addSpacing(space);
	layout->addWidget(mic_volume_slider);
	layout->addSpacing(margin_right);

	widget->setLayout(layout);	
	action->setDefaultWidget(widget);
	mic_menu->addAction(action);	

	float volume = LocalSettings::Instance()->GetMicVolume(AppConfig::default_volume);
	bool mute = LocalSettings::Instance()->GetMicMute(AppConfig::default_mute);
	elogd("[MenuControl::AddMicVolumeMenu] is_enable(%d) volume(%f) mute(%d)", is_enable, volume, mute);

	mic_mute_checkbox->setChecked(mute);
	mic_mute_checkbox->setDisabled(!is_enable);

	mic_volume_slider->setRange(0, 100);
	mic_volume_slider->setValue(volume*100);
	mic_volume_slider->setSaveVolume(volume*100);
	mic_volume_slider->setDisabled(!is_enable);

	auto mute_toggle_handler = [mic_volume_slider](bool checked) {
		elogd("[MenuControl::AddMicVolumeMenu::mute_toggle_handler] checked(%d)", checked);
		bool mute = checked;
		LocalSettings::Instance()->SetMicMute(mute);
		MainWindow::Get()->SetMuted(mute, !NCAudioMixer::Instance()->GetEnableInputCapture());
		float volume = mute ? 0 : NCAudioMixer::Instance()->GetInputCaptureVolume();
		mic_volume_slider->setValue(volume*100);
	};

	auto value_changed_handler = [mic_volume_slider, mic_mute_checkbox](int value) {
		if (value > 0)
		{
			float volume = (float)value / 100;
			LocalSettings::Instance()->SetMicVolume(volume);
			NCAudioMixer::Instance()->SetInputCaptureVolume(volume);
		}

		bool mute = (value == 0);
		if (mute != mic_mute_checkbox->isChecked())
		{
			mic_mute_checkbox->setChecked(mute);
			if (mute)
			{
				LocalSettings::Instance()->SetMicMute(mute);
				MainWindow::Get()->SetMuted(mute, !NCAudioMixer::Instance()->GetEnableInputCapture());
				mic_volume_slider->setValue(0);
			}
		}
	};
	container->connect(mic_mute_checkbox, &QCheckBox::toggled, container, mute_toggle_handler);
	container->connect(mic_volume_slider, &QSlider::valueChanged, container, value_changed_handler);
	mute_toggle_handler(mute);
}

void MenuControl::SetMicIndex(int index)
{
	for (int i=0; i<m_mic_checkbox_list.size(); i++)
	{
		m_mic_checkbox_list[i]->setChecked((i==index));
	}

	if (index==m_mic_list.size())
	{
		// 마이크 사용 안함
		NCAudioMixer::Instance()->SetEnableInputCapture(false);
		LocalSettings::SetSelectedMicDeviceID(QString::fromStdString(AppConfig::not_use_mic_device));
		
		m_mic_mute_checkbox->setDisabled(true);
		m_mic_volume_slider->setDisabled(true);

		bool mute = LocalSettings::Instance()->GetMicMute(AppConfig::default_mute);
		MainWindow::Get()->SetMuted(mute, true);
	}
	else 
	{
		// 마이크 선택
		NCAudioMixer::Instance()->SetEnableInputCapture(true);
		MainWindow::Get()->SetMicSelect(index);
		LocalSettings::SetSelectedMicDeviceID(QString::fromStdString(m_mic_list[index].id));
		
		m_mic_mute_checkbox->setDisabled(false);
		m_mic_volume_slider->setDisabled(false);

		bool mute = LocalSettings::Instance()->GetMicMute(AppConfig::default_mute);
		MainWindow::Get()->SetMuted(mute, false);
	}
}

void MenuControl::SetEnableMask(bool enable)
{
	std::vector<std::wstring> mask_list;
	MaskingManager::Instance()->GetMaskNameList(mask_list);
	for (int i=0; i<m_mask_checkbox_list.size(); i++)
	{
		bool is_mask = enable && m_mask_checkbox_list[i]->isChecked();
		MaskingManager::Instance()->SetVisible(i, is_mask);
		
		QCheckBox* checkbox = m_mask_checkbox_list[i];
		checkbox->setDisabled(!enable);
		checkbox->setProperty("enable_mask", enable ? true : false);
		checkbox->style()->unpolish(checkbox);
		checkbox->style()->polish(checkbox);
	}
	LocalSettings::SetEnableGameMask(enable);
}

void MenuControl::ToggleMaskItem(int index, bool enable)
{
	std::vector<std::wstring> mask_list;
	bool is_enable_game_mask = LocalSettings::GetEnableGameMask(LocalSettings::DEFAULT_ENABLE_GAME_MASK);
	MaskingManager::Instance()->GetMaskNameList(mask_list);
	MaskingManager::Instance()->SetVisible(index, (is_enable_game_mask && enable));
	LocalSettings::SetGameMaskSettings(App()->GetOriginGameCode(), mask_list.size(), index, enable);
}

QString MenuControl::GetQss(eThemeType type)
{
	QString qss{};
	switch(type)
	{
	case eThemeType::eTT_Light:
		qss = GetQssThemeLight();
		break;
	case eThemeType::eTT_Purple:
		qss = GetQssThemeLight();
		break;
	case eThemeType::eTT_Dark:
		qss = GetQssThemeLight();
		break;
	defualt:
		qss = GetQssThemeLight();
	}
	return qss;
}

QString MenuControl::GetQssThemeLight()
{
	return QStringLiteral(
	R"DELIM(

	)DELIM");
}