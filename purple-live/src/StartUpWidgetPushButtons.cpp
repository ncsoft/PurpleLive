#include "StartUpWidgetPushButtons.h"

#include <qpainter.h>

#include "AppUtils.h"
#include "MainWindow.h"
#include "lime-defines.h"
#include "LimeCommunicator.h"
#include "AppString.h"
#include "LocalSettings.h"

CharacterSelectPushButton::CharacterSelectPushButton(QWidget *parent)
	: QPushButton(parent)
{
	m_defaultCharacter = QPixmap(":/StartStreamDlg/image/ic_default_character.png");
	m_arrowIcon = QPixmap(":/StartStreamDlg/image/ic_chevron_right.svg");
	m_printCharacterName = AppString::GetQString("Common/service_name");
}

CharacterSelectPushButton::~CharacterSelectPushButton()
{
}

void CharacterSelectPushButton::SetFixedWidth(QString& name)
{
	int characterNameWidth{};
	int characterNameSuffixWidth{};
	int characterNamePrefixWidth{};

	if (name.isEmpty())
	{
		characterNameWidth = DrawUtils::GetFontWidth(m_textFont, AppString::GetQString("Common/service_name"));
	}
	else
	{
		characterNameWidth = DrawUtils::GetFontWidth(m_textFont, name);
		characterNameSuffixWidth = DrawUtils::GetFontWidth(m_textFont, AppString::GetQString("StreamDlg/default_room_title_suffix"));
		characterNamePrefixWidth = DrawUtils::GetFontWidth(m_textFont, AppString::GetQString("StreamDlg/default_room_title_prefix"));
	}

	setFixedWidth(characterNameWidth + characterNameSuffixWidth + characterNamePrefixWidth + CHARACTER_BTN_ARROW_SIZE + CHARACTER_BTN_NAME_MARGIN);
}

void CharacterSelectPushButton::InitUIAfterCreation()
{
	connect(MainWindow::Get(), &MainWindow::UpdateChildLanguageString, this, &CharacterSelectPushButton::UpdateLanguageString);

	SetFixedWidth(AppString::GetQString("Common/service_name"));

	update();
}

void CharacterSelectPushButton::paintEvent(QPaintEvent *e)
{
	QPainter painter(this);
	int currentIndex = LimeCommunicator::Instance()->GetCurrentCharacterIndex();
	int _y = MainWindow::Get()->GetStartupWindow()->getTitlebarHeight() / 2 - CHARACTER_BTN_IMAGE_SIZE / 2 - 1 /* -1 is border*/;

	if (currentIndex < 0) {
		// 캐릭터 없음.
		painter.setRenderHint(QPainter::Antialiasing);
		DrawUtils::DrawEllipseImage(&m_defaultCharacter, &painter, QRect(0, _y, CHARACTER_BTN_IMAGE_SIZE, CHARACTER_BTN_IMAGE_SIZE));
		m_printCharacterName = AppString::GetQString("Common/service_name");
	}
	else {
		QPixmap* currentSelectImage = MainWindow::Get()->GetStartupWindow()->GetUserImageFromIndex(currentIndex);
		QString name = QString::fromStdString(LimeCommunicator::Instance()->GetCharacterNamefromIndex(currentIndex));
		if (currentSelectImage == nullptr || name.isEmpty()) {
			currentSelectImage = &m_defaultCharacter;
			m_printCharacterName = AppString::GetQString("Common/service_name");
		}
		else
		{
			SetFixedWidth(name);
			m_printCharacterName = name;
		}
		DrawUtils::DrawEllipseImage(currentSelectImage, &painter, QRect(0, _y, CHARACTER_BTN_IMAGE_SIZE, CHARACTER_BTN_IMAGE_SIZE));
	}

	int __y = MainWindow::Get()->GetStartupWindow()->getTitlebarHeight() / 2 - 20 / 2 - 1;

	// draw prefix
	int characterNamePrefixWidth{0};
	if (currentIndex >= 0 && m_printCharacterName != AppString::GetQString("Common/service_name")) {
		characterNamePrefixWidth = DrawUtils::GetFontWidth(m_textFont, AppString::GetQString("StreamDlg/default_room_title_prefix"));
		painter.setPen(m_suffixRoomNameTextColor);
		painter.setFont(m_textFont);
		painter.drawText(30,
			__y,
			DrawUtils::GetFontWidth(m_textFont, AppString::GetQString("StreamDlg/default_room_title_prefix")),
			20, Qt::AlignLeft | Qt::AlignTop, AppString::GetQString("StreamDlg/default_room_title_prefix"));
	}

	painter.setPen(m_characterNameTextColor);
	painter.setFont(m_textFont);

	// draw characterName
	painter.drawText(30 + characterNamePrefixWidth,
		__y, 
		DrawUtils::GetFontWidth(m_textFont, m_printCharacterName),
		20, Qt::AlignLeft | Qt::AlignTop, m_printCharacterName);

	if (currentIndex >= 0 && m_printCharacterName != AppString::GetQString("Common/service_name")) {
		int characterNameWidth = DrawUtils::GetFontWidth(m_textFont, m_printCharacterName);
		int characterNameSuffixWidth = DrawUtils::GetFontWidth(m_textFont, AppString::GetQString("StreamDlg/default_room_title_suffix"));

		// draw suffix
		painter.setPen(m_suffixRoomNameTextColor);
		painter.setFont(m_textFont);
		painter.drawText(30 + characterNameWidth + characterNamePrefixWidth,
			__y,
			DrawUtils::GetFontWidth(m_textFont, AppString::GetQString("StreamDlg/default_room_title_suffix")),
			20, Qt::AlignLeft | Qt::AlignTop, AppString::GetQString("StreamDlg/default_room_title_suffix"));

		// draw arrow
		StartUpWindow* tparent = MainWindow::Get()->GetStartupWindow();

		QPixmap downArrowScaled = m_arrowIcon.scaled(CHARACTER_BTN_ARROW_SIZE, CHARACTER_BTN_ARROW_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		painter.drawPixmap(30 + characterNameSuffixWidth + characterNameWidth + characterNamePrefixWidth + 4,
			tparent->getTitlebarHeight() / 2 - CHARACTER_BTN_ARROW_SIZE / 2 - 1,
			CHARACTER_BTN_ARROW_SIZE,
			CHARACTER_BTN_ARROW_SIZE, downArrowScaled);
	}

	painter.end();
}

void CharacterSelectPushButton::SetCharacterChagned()
{
	UpdateLanguageString();
}

void CharacterSelectPushButton::UpdateLanguageString()
{
	SetFixedWidth(QString::fromStdString(LimeCommunicator::Instance()->GetCharacterNamefromIndex(LimeCommunicator::Instance()->GetCurrentCharacterIndex())));
	update();
}

StartStreamPushButton::StartStreamPushButton(QWidget *parent)
	: QPushButton(parent)
{
	m_printStrWidth = DrawUtils::GetFontWidth(AppUtils::GetBoldFont(STREAM_BTN_TEXT_FONT_SIZE), AppString::GetQString("StreamDlg/share_starting"));
}

StartStreamPushButton::~StartStreamPushButton()
{
}

void StartStreamPushButton::SetPrepareStream(bool enable)
{
	setEnabled(!enable);
	m_prepareStream = enable;
	UpdateLanguageString();

	update();
}

void StartStreamPushButton::showEvent(QShowEvent* e)
{
	m_streamBtnVideoIcon = DrawUtils::ChangeIconColor(m_streamBtnVideoIcon, m_iconTextColor);
	m_streamBtnVideoDisableIcon = DrawUtils::ChangeIconColor(m_streamBtnVideoIcon, m_iconTextDisableColor);
}

void StartStreamPushButton::changeEvent(QEvent *e)
{
	if (e->type() == QEvent::StyleChange)
	{
		m_streamBtnVideoIcon = DrawUtils::ChangeIconColor(m_streamBtnVideoIcon, m_iconTextColor);
		m_streamBtnVideoDisableIcon = DrawUtils::ChangeIconColor(m_streamBtnVideoIcon, m_iconTextDisableColor);
	}
}

void StartStreamPushButton::paintEvent(QPaintEvent *e)
{
	QPushButton::paintEvent(e);

	QPainter painter(this);

	int _y = height() / 2 - STREAM_BTN_ICON_SIZE / 2;

	QPixmap* printIcon;
	QColor* printColor;

	if (m_prepareStream)
	{
		printIcon = &m_streamBtnVideoDisableIcon;
		m_printStr = AppString::GetQString("StreamDlg/share_starting");
		printColor = &QColor(GetIconTextDisableColor());

	}
	else
	{
		m_printStr = AppString::GetQString("StreamDlg/share_start");
		if (this->isEnabled())
		{
			printIcon = &m_streamBtnVideoIcon;
			printColor = &QColor(GetIconTextColor());
		}
		else
		{
			printIcon = &m_streamBtnVideoDisableIcon;
			printColor = &QColor(GetIconTextDisableColor());
		}
		
	}

	int _newTextX = width() / 2 - m_printStrWidth / 2 + STREAM_BTN_ICON_SIZE / 2;
	int _newImageX = _newTextX - STREAM_BTN_ICON_SIZE - STREAM_BTN_TEXT_2_ICON_MARGIN;

	// draw Image
	if (m_streamBtnVideoIcon.isNull() == false)
	{
		DrawUtils::DrawRectImage(printIcon, &painter, QRect(_newImageX, _y, STREAM_BTN_ICON_SIZE, STREAM_BTN_ICON_SIZE));
	}

	// draw Text
	painter.setPen(*printColor);
	painter.setFont(AppUtils::GetBoldFont(STREAM_BTN_TEXT_FONT_SIZE));
	painter.drawText(_newTextX, _y, m_printStrWidth, 20, Qt::AlignLeft, m_printStr);

	painter.end();
}

void StartStreamPushButton::UpdateLanguageString()
{
	if (m_prepareStream) m_printStr = AppString::GetQString("StreamDlg/share_starting");
	else m_printStr = AppString::GetQString("StreamDlg/share_start");
	m_printStrWidth = DrawUtils::GetFontWidth(AppUtils::GetBoldFont(STREAM_BTN_TEXT_FONT_SIZE), m_printStr);
}

SettingPushButton::SettingPushButton(QWidget *parent)
	: QPushButton(parent)
{
	m_chevronIcon = QPixmap(":/StartStreamDlg/image/ic_chevron_right.svg");

	m_printInfo = new QLabel(static_cast<QWidget*>(this));
	m_printInfo->setText("");
	m_printInfo->setObjectName("SettingBtnLabel");
}

SettingPushButton::~SettingPushButton()
{

}

void SettingPushButton::SetChevronRightColor(QColor color)
{ 
	m_chevronRightColor = color;
	m_chevronIcon = DrawUtils::ChangeIconColor(m_chevronIcon, m_chevronRightColor);
}

void SettingPushButton::showEvent(QShowEvent* e)
{
	connect(MainWindow::Get(), &MainWindow::UpdateChildLanguageString, this, &SettingPushButton::UpdateLanguageString);
}

void MicInfoPushButton::UpdateLanguageString()
{
	QString savedMicDeviceID;
	savedMicDeviceID = LocalSettings::GetSelectedMicDeviceID(savedMicDeviceID);

	if (savedMicDeviceID == AppConfig::not_use_mic_device) {
		SetInfoName(AppString::GetQString("Common/un_used"));
	}
}

void MaskInfoPushButton::UpdateLanguageString()
{
	bool is_enable_game_mask = LocalSettings::GetEnableGameMask(LocalSettings::DEFAULT_ENABLE_GAME_MASK);
	if (is_enable_game_mask)
	{
		SetInfoName(AppString::GetQString("StreamDlg/mask_info_on"));
	}
	else
	{
		SetInfoName(AppString::GetQString("StreamDlg/mask_info_off"));
	}
}

void SettingPushButton::SetInfoName(QString text)
{
	int _x = SETTING_BTN_FIXED_WIDTH - 8 - CHEVRON_RIGHT_ICON_SIZE;
	int _y = (SETTING_BTN_FIXED_HEIGHT / 2) - (20 / 2);
	int fontWidth = DrawUtils::GetFontWidth(AppUtils::GetRegularFont(SETTING_BTN_FONT_SIZE), text);
	int maxWidth = SETTING_BTN_FIXED_WIDTH - 30 - CHEVRON_RIGHT_ICON_SIZE - DrawUtils::GetFontWidth(AppUtils::GetMediumFont(SETTING_BTN_FONT_SIZE), AppString::GetQString("StreamDlg/mic_setting_btn"));

	if (fontWidth > maxWidth) {
		for (int i = text.length(); i > 0; i--)
		{
			text = text.mid(0, i);
			text += " ...";
			fontWidth = DrawUtils::GetFontWidth(AppUtils::GetRegularFont(SETTING_BTN_FONT_SIZE), text);
			if (fontWidth < maxWidth) break;
			text = text.mid(0, text.length() - 4);
		}
	}

	m_printInfo->setText(text);
	m_printInfo->setGeometry(_x - fontWidth - 4, _y, fontWidth, 20);
}

void SettingPushButton::paintEvent(QPaintEvent *e)
{
	QPushButton::paintEvent(e);

	// draw icon
	QPainter painter(this);

	int _x = width() - 8 - CHEVRON_RIGHT_ICON_SIZE;
	int _y = (height() / 2) - (CHEVRON_RIGHT_ICON_SIZE / 2) + 1;

	QPixmap downArrowScaled = m_chevronIcon.scaled(CHEVRON_RIGHT_ICON_SIZE, CHEVRON_RIGHT_ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	painter.drawPixmap(_x, _y, CHEVRON_RIGHT_ICON_SIZE, CHEVRON_RIGHT_ICON_SIZE, downArrowScaled);

	painter.end();
}

ClosePushButton::ClosePushButton(QWidget *parent)
	: QPushButton(parent)
{
	m_icon = QPixmap(":/StartStreamDlg/image/ic_light_theme_popup_close.png");
}

ClosePushButton::~ClosePushButton()
{

}

void ClosePushButton::paintEvent(QPaintEvent *e)
{
	QPushButton::paintEvent(e);

	// draw icon
	QPainter painter(this);

	int _x = width() / 2 - m_iconWidth / 2;
	int _y = height() / 2 - m_iconHeight / 2;

	QPixmap _scaled = m_icon.scaled(m_iconWidth, m_iconHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	painter.drawPixmap(_x, _y, m_iconWidth, m_iconHeight, _scaled);

	painter.end();
}

void ClosePushButton::SetIconColor(QColor color)
{
	m_iconColor = color;
	m_icon = DrawUtils::ChangeIconColor(m_icon, m_iconColor);
}