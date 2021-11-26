#include "ShareWithGroupWidget.h"
#include "VideoChatApp.h"
#include "MainWindow.h"
#include "StartUpWindow.h"
#include "AppString.h"
#include "AppUtils.h"

#include <qpainter.h>

ShareWithGroupWidget::ShareWithGroupWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	InitUI();

	connect(ui.gourpUser_listWidget, QOverload<QListWidgetItem*>::of(&QListWidget::itemClicked), this, QOverload<QListWidgetItem*>::of(&ShareWithGroupWidget::ItemClicked));
	ui.gourpUser_listWidget->viewport()->installEventFilter(this);
}

ShareWithGroupWidget::~ShareWithGroupWidget()
{
}

void ShareWithGroupWidget::InitUI()
{
	ui.guidlName_label->setText(App()->GetGuildLabel());
	ui.groupUserCount_label->setText(QString::number(m_selectedUser));
	ui.selectAll_btn->installEventFilter(this);

	m_selectAllOnImage = QPixmap(":/StartStreamDlg/image/ic_checked.svg");
	m_selectAllOffImage = QPixmap(":/StartStreamDlg/image/close.svg");

	ui.gourpUser_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	ui.gourpUser_listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

	m_selectAllBtnFont = QFont(AppUtils::GetMediumFont(GROUPWIDGET_SELECT_ALL_BTN_FONT));

	UpdateLanguageString();
}

void ShareWithGroupWidget::SetMsgPenColor(QColor color)
{ 
	m_msgColor = color; 
	m_selectAllOnImage = DrawUtils::ChangeIconColor(m_selectAllOnImage, m_msgColor);
	m_selectAllOffImage = DrawUtils::ChangeIconColor(m_selectAllOffImage, m_msgColor);
}

void ShareWithGroupWidget::InitUIAfterCreation()
{
	connect(MainWindow::Get(), &MainWindow::UpdateChildLanguageString, this, &ShareWithGroupWidget::UpdateLanguageString);
}

void ShareWithGroupWidget::SetPrintText(ePrintText e)
{ 
	m_printText = e;

	m_emptyUserMsg = AppString::GetQString(AppUtils::format("%s/empty_guild_user", App()->GetOriginGameCode().c_str()).c_str());
	m_emptyGuildMsg = AppString::GetQString(AppUtils::format("%s/empty_guild", App()->GetOriginGameCode().c_str()).c_str());
}

void ShareWithGroupWidget::SetGuildName(QString name)
{ 
	QString guildLabel{ App()->GetGuildLabel() };
	m_guildName = name;

	name += guildLabel;

	int nwidth = DrawUtils::GetFontWidth(AppUtils::GetBoldFont(12), name);
	ui.guidlName_label->setFixedWidth(nwidth);
	ui.guidlName_label->setText(name); 
}

void ShareWithGroupWidget::ItemClicked(QListWidgetItem* item)
{
	if (MainWindow::Get()->GetStartupWindow()->CheckStartingStream()) return;

	int row = ui.gourpUser_listWidget->currentRow();
	GuildUserListItem* guildUserItem = MainWindow::Get()->GetStartupWindow()->GetGuildUserListItem(row);
	if (guildUserItem) {
		if (guildUserItem->GetSelected())
		{
			m_selectedUser--;
			guildUserItem->SetSelected(false);
		}
		else
		{
			m_selectedUser++;
			guildUserItem->SetSelected(true);
		}

		ui.groupUserCount_label->setText(QString::number(m_selectedUser));
		
		if (m_selectedUser <= 0) {
			MainWindow::Get()->GetStartupWindow()->SetStreamBtnDisable(true);
			m_selectedUser = 0;
		}
		else {
			if (MainWindow::Get()->GetStartupWindow()->GetStreamBtnDisable())
			{
				MainWindow::Get()->GetStartupWindow()->SetStreamBtnDisable(false);
			}
		}

		ui.groupUserCount_label->update();
		ui.selectAll_btn->update();
		guildUserItem->update();
	}
}

bool ShareWithGroupWidget::eventFilter(QObject* obj, QEvent* event)
{
	if (obj == ui.selectAll_btn)
	{
		if (event->type() == QEvent::Paint)
		{
			QPainter painter(ui.selectAll_btn);

			QRect r(ui.selectAll_btn->width() - GROUPWIDGET_SELECT_ICON_SIZE - 8,
				ui.selectAll_btn->height() / 2 - GROUPWIDGET_SELECT_ICON_SIZE / 2,
				GROUPWIDGET_SELECT_ICON_SIZE,
				GROUPWIDGET_SELECT_ICON_SIZE);

			QString printText;

			if (m_selectedUser)
			{
				DrawUtils::DrawRectImage(&m_selectAllOffImage, &painter, r);
				printText = AppString::GetQString("ShareWithGroupDlg/user_all_select_cancel_btn");
			}
			else
			{
				DrawUtils::DrawRectImage(&m_selectAllOnImage, &painter, r);
				printText = AppString::GetQString("ShareWithGroupDlg/user_all_select_ok_btn");
			}

			int printTextWidth = DrawUtils::GetFontWidth(AppUtils::GetMediumFont(GROUPWIDGET_SELECT_ALL_BTN_FONT), printText);
			ui.selectAll_btn->setFixedWidth(printTextWidth + GROUPWIDGET_SELECT_ALL_BTN_LEFT_MARGIN + GROUPWIDGET_SELECT_ALL_BTN_RIGHT_MARGIN);

			painter.setPen(m_selectAllBtnColor);
			painter.setFont(m_selectAllBtnFont);
			painter.drawText(12, 
				ui.selectAll_btn->height() / 2 - 20 / 2, 
				printTextWidth, 
				16, 
				Qt::AlignLeft, printText);

			painter.end();
		}
	}
	else if (obj == ui.gourpUser_listWidget->viewport())
	{
		if (event->type() == QEvent::Paint)
		{
			QString printString{};

			if (m_printText == ePrintText::emptyUser)
			{
				printString = m_emptyUserMsg;
			}
			else if (m_printText == ePrintText::emptyGuild)
			{
				printString = m_emptyGuildMsg;
			}
			else
			{
				return QWidget::eventFilter(obj, event);
			}

			QPainter painter(ui.gourpUser_listWidget->viewport());

			painter.setPen(m_msgColor);
			painter.setFont(AppUtils::GetMediumFont(13));
			painter.drawText(-(this->x() - 10),
				ui.gourpUser_listWidget->viewport()->height() / 2 - 40 / 2,
				width(),
				40, Qt::AlignHCenter, printString);

			painter.end();
		}
	}

	return QWidget::eventFilter(obj, event);
}

void ShareWithGroupWidget::clearUser()
{
	ui.gourpUser_listWidget->clear();
	m_selectedUser = 0;
	MainWindow::Get()->GetStartupWindow()->SetStreamBtnDisable(true);
	ui.groupUserCount_label->setText(QString::number(m_selectedUser));
	ui.groupUserCount_label->update();
}

void ShareWithGroupWidget::UpdateProfileImage(int index)
{
	QNetworkAccessManager *manager = new QNetworkAccessManager(this);
	connect(manager, &QNetworkAccessManager::finished, this, &ShareWithGroupWidget::ProfileImageDownloadFinished);
	connect(manager, &QNetworkAccessManager::finished, manager, &QNetworkAccessManager::deleteLater);

	GuildUserListItem* guildUserItem = MainWindow::Get()->GetStartupWindow()->GetGuildUserListItem(index);

	if (!guildUserItem) return;

	const QUrl url = QUrl(guildUserItem->GetUserProfileUrl().c_str());

	QNetworkRequest request(url);
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

	QNetworkReply* reply = manager->get(request);
	reply->setObjectName(QString::number(index));
}

void ShareWithGroupWidget::ProfileImageDownloadFinished(QNetworkReply *reply)
{
	int profileImageSize = GUILDUSER_IMAGE_SIZE;
	QPixmap pm;
	if (!pm.loadFromData(reply->readAll())) {
		pm.load(":/StartStreamDlg/image/ic_default_character.png");
	}
	pm = pm.scaled(profileImageSize, profileImageSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

	QImage dst(profileImageSize, profileImageSize, QImage::Format_ARGB32);
	dst.fill(QColor(0, 0, 0, 0));

	QPainter p(&dst);
	QPainterPath path;
	path.addRoundedRect(0, 0, profileImageSize, profileImageSize, profileImageSize / 2, profileImageSize / 2);

	p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, true);

	QPen pen(GUILDUSER_IMAGE_BORDER_COLOR, GUILDUSER_IMAGE_BORDER);
	p.setPen(pen);

	p.fillPath(path, QBrush(Qt::white));

	p.setCompositionMode(QPainter::CompositionMode_SourceAtop);

	p.drawPixmap(dst.rect(), pm);
	p.drawPath(path);

	int index = reply->objectName().toInt();

	GuildUserListItem* guildUserItem = MainWindow::Get()->GetStartupWindow()->GetGuildUserListItem(index);
	if (!guildUserItem) return;
	guildUserItem->SetUserProfileImage(QPixmap::fromImage(dst));
	guildUserItem->update();
}

void ShareWithGroupWidget::on_selectAll_btn_clicked()
{
	if (m_selectedUser != 0 || ui.gourpUser_listWidget->count() == 0) {
		MainWindow::Get()->GetStartupWindow()->SelectAllGuildUser(false);
		m_selectedUser = 0;
		MainWindow::Get()->GetStartupWindow()->SetStreamBtnDisable(true);
	}
	else {
		MainWindow::Get()->GetStartupWindow()->SelectAllGuildUser(true);
		m_selectedUser = MainWindow::Get()->GetStartupWindow()->GetGuildUserCount();
		MainWindow::Get()->GetStartupWindow()->SetStreamBtnDisable(false);
	}

	ui.groupUserCount_label->setText(QString::number(m_selectedUser));
	ui.gourpUser_listWidget->update();
	ui.selectAll_btn->update();
}

void ShareWithGroupWidget::HideLabelWidget(bool bhide)
{
	if (bhide)
	{
		ui.labelWidget->hide();
	}
	else
	{
		ui.labelWidget->show();
	}
}

void ShareWithGroupWidget::UpdateLanguageString()
{
	QString prefix = AppString::GetQString("ShareWithGroupDlg/user_count_prefix_label");
	int _newHeight{};
	if (!prefix.isEmpty())
	{
		int fontWidth = DrawUtils::GetFontWidth(AppUtils::GetBoldFont(14), prefix);
		ui.label_usercount_prefix->setText(prefix);
		ui.label_usercount_prefix->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		ui.label_usercount_prefix->setFixedWidth(fontWidth + 4);
		_newHeight = GROUPWIDGET_GOURP_COUNT_LABEL_HEIGHT;
	}
	else
	{
		ui.label_usercount_prefix->setFixedWidth(0);
	}
	ui.label_usercount_prefix->setFixedHeight(_newHeight);

	ui.label_2->setText(AppString::GetQString("ShareWithGroupDlg/user_count_suffix_label"));
	SetGuildName(m_guildName);
	m_emptyUserMsg = AppString::GetQString(AppUtils::format("%s/empty_guild_user", App()->GetOriginGameCode().c_str()).c_str());
	m_emptyGuildMsg = AppString::GetQString(AppUtils::format("%s/empty_guild", App()->GetOriginGameCode().c_str()).c_str());

	update();
}