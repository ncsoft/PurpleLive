#pragma once

#include <qwidget.h>
#include <qurl.h>
#include <qnetworkaccessmanager.h>

#include "ui_ShareWithGroupWidget.h"

#define GROUPWIDGET_SELECT_ICON_SIZE				16
#define GROUPWIDGET_GOURP_COUNT_LABEL_HEIGHT		12
#define GROUPWIDGET_SELECT_ALL_BTN_FONT				12
#define GROUPWIDGET_SELECT_ALL_BTN_LEFT_MARGIN		12
#define GROUPWIDGET_SELECT_ALL_BTN_RIGHT_MARGIN		26

enum class ePrintText {
	none,
	emptyUser,
	emptyGuild
};

class ShareWithGroupWidget : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(QColor selectAllBtnColor WRITE SetSelectAllBtnColor READ GetSelectAllBtnColor)
	Q_PROPERTY(QColor MsgPenColor WRITE SetMsgPenColor READ GetMsgPenColor)

public:
	ShareWithGroupWidget(QWidget *parent = Q_NULLPTR);
	~ShareWithGroupWidget();

	void UpdateProfileImage(int index);
	void HideLabelWidget(bool bhide);
	void clearUser();
	void SetGuildName(QString name);
	void SetPrintText(ePrintText e);
	void InitUIAfterCreation();
	VideoChatQListWidget* GetListWidget() { return ui.gourpUser_listWidget; }
	int GetUserCount() { return ui.gourpUser_listWidget->count(); }
	int GetLabelWidgetHeight() { return ui.labelWidget->height(); }
	int GetSelectedUserCount() { return m_selectedUser; }

private:
	Ui::ShareWithGroupWidget ui;

	int m_selectedUser = 0;
	QPixmap m_selectAllOnImage;
	QPixmap m_selectAllOffImage;
	QFont m_selectAllBtnFont;
	QColor m_selectAllBtnColor;

	ePrintText m_printText;
	QColor m_msgColor;
	QString m_emptyUserMsg;
	QString m_emptyGuildMsg;
	QString m_guildName;

	void InitUI();
	void ProfileImageDownloadFinished(QNetworkReply *reply);

private slots:
	void on_selectAll_btn_clicked();
	void UpdateLanguageString();
	void ItemClicked(QListWidgetItem* item);
	bool eventFilter(QObject* obj, QEvent* event);

	void SetSelectAllBtnColor(QColor color) { m_selectAllBtnColor = color; }
	QColor GetSelectAllBtnColor() { return m_selectAllBtnColor; }
	void SetMsgPenColor(QColor color);
	QColor GetMsgPenColor() { return m_msgColor; }
};