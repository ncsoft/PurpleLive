#pragma once

#include <qpushbutton.h>
#include <qfontmetrics.h>
#include <qlabel.h>
#include <qevent.h>

#define		CHARACTER_BTN_NAME_LEFT_MARGIN		30
#define		CHARACTER_BTN_NAME_RIGHT_MARGIN		14
#define		CHARACTER_BTN_NAME_MARGIN			(CHARACTER_BTN_NAME_LEFT_MARGIN + CHARACTER_BTN_NAME_RIGHT_MARGIN)
#define		CHARACTER_BTN_IMAGE_SIZE			24
#define		CHARACTER_BTN_ARROW_SIZE			16
#define		CHARACTER_NAME_FONT					13

#define		STREAM_BTN_ICON_SIZE				24
#define		STREAM_BTN_TEXT_2_ICON_MARGIN		4
#define		STREAM_BTN_TEXT_FONT_SIZE			14

#define		CHEVRON_RIGHT_ICON_SIZE				16
#define		SETTING_BTN_FIXED_WIDTH				360
#define		SETTING_BTN_FIXED_HEIGHT			40
#define		SETTING_BTN_FONT_SIZE				14
#define		SETTING_BTN_FONT_COLOR				QColor(23, 23, 45, 255*0.8)

#define		CLOSE_BTN_ICON_SIZE					24

class SettingPushButton : public QPushButton
{
	Q_OBJECT
	Q_PROPERTY(QColor chevronRight WRITE SetChevronRightColor READ GetChevronRightColor)

public:
	explicit SettingPushButton(QWidget *parent = Q_NULLPTR);
	~SettingPushButton();

	void SetLabelText(QString text) { setText(text); }
	void SetInfoName(QString text);


protected slots:
	virtual void UpdateLanguageString() {}

private:
	QPixmap m_chevronIcon{};
	QLabel* m_printInfo = Q_NULLPTR;

	virtual void showEvent(QShowEvent* e) override;
	virtual void paintEvent(QPaintEvent *e) override;

	// q-property value, func
	QColor m_chevronRightColor;

	void SetChevronRightColor(QColor color);
	QColor GetChevronRightColor() { return m_chevronRightColor; }
};

class MicInfoPushButton : public SettingPushButton
{
	Q_OBJECT

public:
	explicit MicInfoPushButton(QWidget *parent = Q_NULLPTR) {}
	~MicInfoPushButton() {}

public slots:
	virtual void UpdateLanguageString() override;
};


class MaskInfoPushButton : public SettingPushButton
{
	Q_OBJECT

public:
	explicit MaskInfoPushButton(QWidget *parent = Q_NULLPTR) {}
	~MaskInfoPushButton() {}

public slots:
	virtual void UpdateLanguageString() override;
};

class StartStreamPushButton : public QPushButton
{
	Q_OBJECT
	Q_PROPERTY(QColor iconTextColor WRITE SetIconTextColor READ GetIconTextColor)
	Q_PROPERTY(QColor iconTextDisableColor WRITE SetIconTextDisableColor READ GetIconTextDisableColor)
	Q_PROPERTY(QPixmap streamIcon WRITE SetStreamIcon READ GetStreamIcon)

public:
	explicit StartStreamPushButton(QWidget *parent = Q_NULLPTR);
	~StartStreamPushButton();

	void SetPrepareStream(bool enable);
	void UpdateLanguageString();

private:
	int m_printStrWidth{};
	QString m_printStr{};
	QPixmap m_streamBtnVideoDisableIcon{};
	bool	m_prepareStream = false;

	// Q-property
	QPixmap m_streamBtnVideoIcon{};
	QColor m_iconTextColor;
	QColor m_iconTextDisableColor;

	void SetIconTextColor(QColor val) { m_iconTextColor = val; }
	void SetIconTextDisableColor(QColor val) { m_iconTextDisableColor = val; }
	void SetStreamIcon(QPixmap val) { m_streamBtnVideoIcon = val; }
	QColor GetIconTextColor() { return m_iconTextColor; }
	QColor GetIconTextDisableColor() { return m_iconTextDisableColor; }
	QPixmap GetStreamIcon() { return m_streamBtnVideoIcon; }

	// Qt Event
	virtual void paintEvent(QPaintEvent *e) override;
	virtual void showEvent(QShowEvent* e) override;
	virtual void changeEvent(QEvent *e) override;
};

class CharacterSelectPushButton : public QPushButton
{
	Q_OBJECT
	Q_PROPERTY(QColor characterName WRITE SetCharacterNameColor READ GetCharacterNameColor)
	Q_PROPERTY(QColor roomName WRITE SetRoomNameColor READ GetRoomNameColor)
	Q_PROPERTY(QFont textFont WRITE SetTextFont READ GetTextFont)

public:
	explicit CharacterSelectPushButton(QWidget *parent = Q_NULLPTR);
	~CharacterSelectPushButton();

	void InitUIAfterCreation();
	void SetCharacterChagned();

private:
	QString m_printCharacterName{};
	QPixmap m_defaultCharacter{};
	QPixmap m_arrowIcon{};

	void SetFixedWidth(QString& name);
	virtual void paintEvent(QPaintEvent *e) override;

	// q-property value, func
	QColor m_characterNameTextColor;
	QColor m_suffixRoomNameTextColor;
	QFont m_textFont;

	void SetCharacterNameColor(QColor color) { m_characterNameTextColor = color; }
	QColor GetCharacterNameColor() { return m_characterNameTextColor; }
	void SetRoomNameColor(QColor color) { m_suffixRoomNameTextColor = color; }
	QColor GetRoomNameColor() { return m_suffixRoomNameTextColor; }
	void SetTextFont(QFont font) { m_textFont = font; }
	QFont GetTextFont() { return m_textFont; }

private slots:
	void UpdateLanguageString();
};

class ClosePushButton : public QPushButton
{
	Q_OBJECT
	Q_PROPERTY(QColor iconColor WRITE SetIconColor READ GetIconColor)

public:
	explicit ClosePushButton(QWidget *parent = Q_NULLPTR);
	~ClosePushButton();

	void SetIconSize(int _width, int _height) { m_iconWidth = _width; m_iconHeight = _height; }

private:
	virtual void paintEvent(QPaintEvent *e) override;

	QPixmap m_icon{};
	int m_iconWidth = CLOSE_BTN_ICON_SIZE;
	int m_iconHeight = CLOSE_BTN_ICON_SIZE;

	// q-property value, func
	QColor m_iconColor;

	void SetIconColor(QColor color);
	QColor GetIconColor() { return m_iconColor; }
};