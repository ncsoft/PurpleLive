#include "ChatListItem.h"

#include "VideoChatApp.h"
#include "StartUpWindow.h"
#include "AppString.h"
#include "AppUtils.h"
#include "MainWindow.h"

#include <qpainter.h>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsEffect>

ChatListItem::ChatListItem(QWidget *parent, int index ,bool isMakeNewChat)
	: QWidget(parent), m_index(index),m_makeNewChat(isMakeNewChat)
{
	ui.setupUi(this);

	ui.changeSubChat_btn->installEventFilter(this);
	ui.chatName_label->setText("");

	m_selectedCheck = QPixmap(":/StartStreamDlg/image/ic_checked.svg");
	m_overCheck = QPixmap(":/StartStreamDlg/image/ic_checked.svg");
	m_newChatProfile = QPixmap(":/StartStreamDlg/image/ic_new_chat.png");
	m_defaultProfile = QPixmap(":/StartStreamDlg/image/ic_default_chat_list_icon.svg");
	m_arrowDown = QPixmap(":/StartStreamDlg/image/ic_arrow_down.svg");
	m_arrowUpActive = QPixmap(":/StartStreamDlg/image/ic_arrow_up.svg");
	m_chatNameFont = AppUtils::GetMediumFont(CHAT_NAME_LABEL_FONT_SIZE);
	m_subChatBtnFont = AppUtils::GetMediumFont(CHAT_NAME_BTN_FONT_SIZE);
	ui.changeSubChat_btn->setText("");

	connect(MainWindow::Get(), &MainWindow::UpdateChildLanguageString, this, &ChatListItem::UpdateLanguageString);
}

ChatListItem::~ChatListItem()
{
	qDeleteAll(m_vecChatRoomInfo);
}

void ChatListItem::SetSelected(bool selected)
{ 
	m_selected = selected; 
	MakePopUpWidget();
}

QString ChatListItem::GetGroupUserID()
{
	if (m_selected) {
		if (m_chatListPopup) {
			if (m_vecChatRoomInfo.size() <= m_chatListPopup->GetCurrentIndex()) return "";
			return m_vecChatRoomInfo[m_chatListPopup->GetCurrentIndex()]->groupUserId;
		}
	}
	return "";
}

QString ChatListItem::GetChannelID()
{
	if (m_selected) {
		if (m_chatListPopup) {
			if (m_vecChatRoomInfo.size() <= m_chatListPopup->GetCurrentIndex()) return "";
			return m_vecChatRoomInfo[m_chatListPopup->GetCurrentIndex()]->channelId;
		}
	}
	return "";
}

void ChatListItem::SelectChannel(std::string groupId, std::string channelid)
{
	if (m_chatListPopup) {
		QString qGroupId = QString::fromStdString(groupId);
		QString qChannelId = QString::fromStdString(channelid);

		if (m_gourpId != qGroupId) return;

		for (int i = 0; i < m_vecChatRoomInfo.size(); i++)
		{
			if (m_vecChatRoomInfo[i]->channelId == qChannelId)
			{
				QListWidgetItem* item = m_chatListPopup->GetPopUpList()->item(i);
				m_chatListPopup->itemClicked(item);
				return;
			}
		}
	}
}

bool ChatListItem::CheckHaveChannelID(const QString& channelid)
{
	for(auto item : m_vecChatRoomInfo)
	{ 
		if (item->channelId == channelid) return true;
	}
	
	return false;
}

void ChatListItem::SetGroupInfo(GroupInfo* gi)
{
	if (m_makeNewChat)
	{
		SetChatName(AppString::GetQString("ChatListItem/create_new_chat"));
		ui.changeSubChat_btn->hide();
		return;
	}

	UpdateProfileImage(gi->groupImageUrl);
	QString game_channel_type = QString::fromUtf8(gi->gameChannelType.c_str());
	m_gourpId = QString::fromUtf8(gi->groupId.c_str());

	for (int j = 0; j < gi->shareChannelList.size(); j++) {
		m_chatRoomInfo = new ChatRoomInfo;
		m_chatRoomInfo->labelName = QString::fromUtf8(gi->name.c_str());
		m_chatRoomInfo->bLabelTitle = false;
		m_chatRoomInfo->gameChannelType = game_channel_type;
		m_chatRoomInfo->groupUserId = QString::fromUtf8(gi->shareChannelList[j].groupUserId.c_str());
		m_chatRoomInfo->channelId = QString::fromUtf8(gi->shareChannelList[j].channelId.c_str());
		m_chatRoomInfo->chatRoomName = QString::fromUtf8(gi->shareChannelList[j].name.c_str());
		if (QString::fromUtf8(gi->shareChannelList[j].channelType.c_str()) == "TEXT") {
			m_chatRoomInfo->bVoiceRoom = false;
		}
		else if (QString::fromUtf8(gi->shareChannelList[j].channelType.c_str()) == "VOICE") {
			m_chatRoomInfo->bVoiceRoom = true;
		}
		else {
			continue;
		}
		m_vecChatRoomInfo.push_back(m_chatRoomInfo);
	}
		
	if (m_vecChatRoomInfo.size() <= 1) {
		ui.changeSubChat_btn->hide();
	}
	else {
		SetChatName(m_chatRoomInfo->labelName);
		SetBtnText(m_vecChatRoomInfo[0]->chatRoomName);
	}
}

void ChatListItem::SetChatName(QString name)
{
	m_originChatRoomName = name;
	int max_width = CHAT_LABEL_MAX_WIDTH - ui.changeSubChat_btn->width();

	m_chatNameWidth = DrawUtils::GetFontWidth(AppUtils::GetMediumFont(CHAT_NAME_LABEL_FONT_SIZE), name);
	if (m_chatNameWidth > max_width) {
		
		for (int i = name.length(); i > 0; i--)
		{
			QString newName = name.mid(0, i);
			newName += "...";
			int newWidth = DrawUtils::GetFontWidth(AppUtils::GetMediumFont(CHAT_NAME_LABEL_FONT_SIZE), newName);

			if (newWidth < max_width)
			{
				m_printChatRoomName = newName;
				m_chatNameWidth = newWidth;
				break;
			}
		}
	}
	else
	{
		m_printChatRoomName = name;
	}

	ui.chatName_label->setFixedWidth(m_chatNameWidth);
}

void ChatListItem::SetBtnText(QString& text)
{
	QString str = text;
	int btnFontWidth = DrawUtils::GetFontWidth(AppUtils::GetMediumFont(CHAT_NAME_BTN_FONT_SIZE), str);

	int max_width = DrawUtils::GetFontWidth(AppUtils::GetRegularFont(CHAT_NAME_LABEL_FONT_SIZE), m_originChatRoomName) > CHAT_ROOM_NAME_MAX_WIDTH ? 
		CHAT_CHANNEL_BTN_MAX_WIDTH : CHAT_LABEL_MAX_WIDTH - ui.chatName_label->width();

	if (btnFontWidth > max_width)
	{
		QString newStr;

		for (int i = str.length(); i > 0; i--)
		{
			newStr = str.mid(0, i);
			newStr += "...";
			int newWidth = DrawUtils::GetFontWidth(AppUtils::GetMediumFont(CHAT_NAME_BTN_FONT_SIZE), newStr);

			if (newWidth < max_width)
			{
				btnFontWidth = newWidth;
				str = newStr;
				break;
			}
		}
	}

	ui.changeSubChat_btn->setFixedWidth(btnFontWidth + CHAT_CHANNEL_BTN_WIDTH_MARGIN);
	m_subchatFontWidth = btnFontWidth;
	m_subchatPrintStr = str;
}

QString ChatListItem::GetChatName()
{
	return m_printChatRoomName;
}

QString ChatListItem::GetSubChatName()
{
	if (ui.changeSubChat_btn->isHidden()) return "";
	else return ui.changeSubChat_btn->text();
}

void ChatListItem::SetMakeNewChaticonColor(QColor color)
{
	m_makeNewChaticonColor = color;
	m_newChatProfile = DrawUtils::ChangeIconColor(m_newChatProfile, m_makeNewChaticonColor);
}

void ChatListItem::paintEvent(QPaintEvent *e)
{
	QWidget::paintEvent(e);

	QPainter painter(this);

	if (m_makeNewChat) {
		// draw nothing
	}
	else if (m_selected) {
		int _x = width() - CHECK_ICON_X_MARGIN - CHECK_ICON_SIZE;
		int _y = (height() / 2) - (CHECK_ICON_SIZE / 2);

		DrawUtils::DrawRectImage(&m_selectedCheck, &painter, QRect(_x, _y, CHECK_ICON_SIZE, CHECK_ICON_SIZE));
	}
	else if (m_isOvered) {
		// draw over icon
		int _x = width() - CHECK_ICON_X_MARGIN - CHECK_ICON_SIZE;
		int _y = (height() / 2) - (CHECK_ICON_SIZE / 2);
		DrawUtils::DrawRectImage(&m_overCheck, &painter, QRect(_x, _y, CHECK_ICON_SIZE, CHECK_ICON_SIZE));
	}
	else
	{ }

	int _y = (height() / 2) - (PROFILE_ICON_SIZE / 2);
	if (m_makeNewChat) {
		DrawUtils::DrawRectImage(&m_newChatProfile, &painter, QRect(PROFILE_ICON_X_MARGIN, _y, PROFILE_ICON_SIZE, PROFILE_ICON_SIZE));
	}
	else if (!m_profileDownloaded) {
		DrawUtils::DrawRectImage(&m_defaultProfile, &painter, QRect(PROFILE_ICON_X_MARGIN, _y, PROFILE_ICON_SIZE, PROFILE_ICON_SIZE));
	}
	else {
		DrawUtils::DrawSquicleImage(&m_chatProfile, &m_defaultProfile, &painter, QRect(PROFILE_ICON_X_MARGIN, _y, PROFILE_ICON_SIZE, PROFILE_ICON_SIZE));
	}

	if (m_makeNewChat) {
		painter.setPen(m_makeNewChatNameColor);
	}
	else {
		painter.setPen(m_chatNameColor);
	}
	
	painter.setFont(m_chatNameFont);

	painter.drawText(48,
		height() / 2 - 11,
		m_chatNameWidth,
		20, Qt::AlignLeft, m_printChatRoomName);

	painter.end();
}

void ChatListItem::SetChatListProfile(QPixmap& profile)
{
	m_profileDownloaded = true;
	m_chatProfile = profile;
	update();
}

void ChatListItem::SetChatListProfileFail()
{
	m_profileDownloaded = false;
}

QPixmap ChatListItem::GetChatListProfile()
{
	if(m_chatProfile.isNull() == false) return m_chatProfile;
	else return nullptr;
}

void ChatListItem::MakePopUpWidget()
{
	HidePopUpWidget();

	if (!m_chatListPopup) 
	{
		m_chatListPopup = make_shared<ChatListPopUpWidget>(MainWindow::Get()->GetStartupWindow(), this);
		m_chatListPopup->SetRoomInfo(GetRoomInfo());
		m_chatListPopup->installEventFilter(this);
	}
}

void ChatListItem::on_changeSubChat_btn_clicked()
{
	MakePopUpWidget();

	ui.changeSubChat_btn->update();

	QRect btnRect = ui.changeSubChat_btn->geometry();
	btnRect.setY(btnRect.y() + btnRect.height() + 5);

	QPoint p = mapToGlobal(QPoint(btnRect.x(), btnRect.y()));

	ShowSubChatPopUp(p.x(), p.y());
}

void ChatListItem::ShowSubChatPopUp(int _x, int _y)
{
	MainWindow::Get()->GetStartupWindow()->HideChatListItemPopUp();
	if (!m_chatListPopup) return;

	m_chatListPopup->SaveCurrentRow();

	QRect btnRect;
	btnRect.setX(_x);
	btnRect.setY(_y);
	btnRect.setWidth(m_chatListPopup->GetMaximumWidth() + CHATLIST_POPUP_WIDTH_MARGIN);
	int _height = CHATLIST_POPUP_MAX_HEIGHT;
	if (CHATLIST_POPUP_MAX_HEIGHT > m_chatListPopup->GetRoomCount() * CHATLIST_POPUP_ITEM_HEIGHT + CHATLIST_POPUP_HEIGHT_MARGIN)
	{
		_height = m_chatListPopup->GetRoomCount() * CHATLIST_POPUP_ITEM_HEIGHT + CHATLIST_POPUP_HEIGHT_MARGIN;
	}
	btnRect.setHeight(_height);
	m_chatListPopup->SetGeometry(btnRect);

	m_chatListPopup->show();
}

bool ChatListItem::eventFilter(QObject* obj, QEvent* event)
{
	if (obj == m_chatListPopup.get())
	{
		if (event->type() == QEvent::WindowDeactivate)
		{
			HidePopUpWidget();
		}
	}
	else if (obj == ui.changeSubChat_btn)
	{
		if (event->type() == QEvent::Paint)
		{
			QPainter painter(ui.changeSubChat_btn);

			// print down arrow icon
			int _x = ui.changeSubChat_btn->width() - 12 - 6;
			int _y = (ui.changeSubChat_btn->height() / 2) - 6;
			QPixmap* drawPixmap;
			QColor drawTextColor;

			if (m_chatListPopup)
			{
				if (m_chatListPopup->isHidden()) {
					
					drawPixmap = &m_arrowDown;
					drawTextColor = m_subchatBtnNormal;
				}
				else {
					drawPixmap = &m_arrowUpActive;
					drawTextColor = m_subchatBtnActive;
				}
			}
			else
			{
				drawPixmap = &m_arrowDown;
				drawTextColor = m_subchatBtnNormal;
			}
			
			DrawUtils::DrawRectImage(drawPixmap, &painter, QRect(_x, _y, 12, 12));

			// print text
			painter.setPen(drawTextColor);
			painter.setFont(m_subChatBtnFont);
			painter.drawText(8, 1, m_subchatFontWidth, 16, Qt::AlignCenter, m_subchatPrintStr);

			painter.end();
		}
	}

	return QWidget::eventFilter(obj, event);
}

bool ChatListItem::event(QEvent *event)
{
	switch (event->type())
	{
	case QEvent::Enter:
		m_isOvered = true;
		break;
	case QEvent::Leave:
		m_isOvered = false;
		break;
	case QEvent::Move:
		HidePopUpWidget();
		break;
	case QEvent::MouseButtonPress:
	case QEvent::WindowDeactivate:
		HidePopUpWidget();
		break;
	}

	return QWidget::event(event);
}

void ChatListItem::HidePopUpWidget()
{
	MainWindow::Get()->GetStartupWindow()->HideChatListItemPopUp();
	ui.changeSubChat_btn->update();
}

std::shared_ptr<ChatListPopUpWidget> ChatListItem::GetChatListPopUp()
{
	if (m_chatListPopup) return m_chatListPopup;
	else return Q_NULLPTR;
}

void ChatListItem::ResetSelect(bool isSelected)
{
	MainWindow::Get()->GetStartupWindow()->ChatListItemRowChanged(m_index);
}

void ChatListItem::UpdateProfileImage(string strUrl)
{
	QNetworkAccessManager *manager = new QNetworkAccessManager(this);
	connect(manager, &QNetworkAccessManager::finished, this, &ChatListItem::ProfileImageDownloadFinished);
	connect(manager, &QNetworkAccessManager::finished, manager, &QNetworkAccessManager::deleteLater);

	const QUrl url = QUrl(strUrl.c_str());

	QNetworkRequest request(url);
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

	QNetworkReply* reply = manager->get(request);
}

void ChatListItem::ProfileImageDownloadFinished(QNetworkReply *reply)
{
	int profileImageSize = PROFILE_ICON_SIZE;
	QPixmap pm;
	if (!pm.loadFromData(reply->readAll())) {
		SetChatListProfileFail();
		return;
	}
	pm = pm.scaled(profileImageSize, profileImageSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

	QImage dst(profileImageSize, profileImageSize, QImage::Format_ARGB32);
	dst.fill(QColor(0, 0, 0, 0));

	QPainter p(&dst);
	QPainterPath path;
	path.addRoundedRect(0, 0, profileImageSize, profileImageSize, PROFILE_RADIUS, PROFILE_RADIUS);

	p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, true);

	QPen pen(CHARACTER_IMAGE_BORDER_COLOR, CHARACTER_IMAGE_BORDER);
	p.setPen(pen);

	p.fillPath(path, QBrush(Qt::white));

	p.setCompositionMode(QPainter::CompositionMode_SourceAtop);

	p.drawPixmap(dst.rect(), pm);
	p.drawPath(path);
	SetChatListProfile(QPixmap::fromImage(dst));
}

void ChatListItem::SetSubchatBtnNormalColor(QColor color)
{
	m_subchatBtnNormal = color;
	m_arrowDown = DrawUtils::ChangeIconColor(m_arrowDown, m_subchatBtnNormal);
}

void ChatListItem::SetSubchatBtnActiveColor(QColor color)
{
	m_subchatBtnActive = color;
}

void ChatListItem::SetSubchatIconActiveColor(QColor color)
{
	m_subchatIconActive = color;
	m_arrowUpActive = DrawUtils::ChangeIconColor(m_arrowUpActive, m_subchatIconActive);
}

void ChatListItem::SetCheckIconHoverColor(QColor color)
{
	m_checkIconHover = color;
	m_overCheck = DrawUtils::ChangeIconColor(m_overCheck, m_checkIconHover);
}

void ChatListItem::SetCheckIconActiveColor(QColor color)
{
	m_checkIconActive = color;
	m_selectedCheck = DrawUtils::ChangeIconColor(m_selectedCheck, m_checkIconActive);
}

void ChatListItem::UpdateLanguageString()
{
	if (m_makeNewChat)
	{
		SetChatName(AppString::GetQString("ChatListItem/create_new_chat"));
		update();
	}
}

ChatListPopUpWidget::ChatListPopUpWidget(QWidget *parent, QWidget *itemParent)
	: QFramelessPopup(parent)
{
	ui.setupUi(this);
	ui.shadowWidget->setAttribute(Qt::WA_TranslucentBackground);

	QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect(ui.ChatListPopUpWidget);
	effect->setBlurRadius(SHADOW_BLUR_RADIUS);
	effect->setColor(SHADOW_COLOR);
	effect->setOffset(QPointF(0, SHADOW_Y_OFFSET));
	ui.ChatListPopUpWidget->setGraphicsEffect(effect);

	setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);

	if (itemParent)
		m_itemParentWidget = static_cast<ChatListItem*>(itemParent);

	ui.ChatListPopUpWidget->connect(ui.ChatListPopUpWidget, QOverload<QListWidgetItem*>::of(&QListWidget::itemClicked), this, QOverload<QListWidgetItem*>::of(&ChatListPopUpWidget::itemClicked));

	m_headsetImage = QPixmap(":/StartStreamDlg/image/ic_popup_headset.svg");
	m_chatImage = QPixmap(":/StartStreamDlg/image/ic_popup_chat.svg");

	ui.ChatListPopUpWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

ChatListPopUpWidget::~ChatListPopUpWidget()
{
}

void ChatListPopUpWidget::SetIconColor(QColor color)
{
	m_iconColor = color;
	m_headsetImage = DrawUtils::ChangeIconColor(m_headsetImage, m_iconColor);
	m_chatImage = DrawUtils::ChangeIconColor(m_chatImage, m_iconColor);

	if (m_itemParentWidget)
	{
		ChangeIconColor();
		m_itemParentWidget->SelectChannel(App()->GetGroupId(), App()->GetChannelId());
	}

	update();
}

void ChatListPopUpWidget::SetGeometry(QRect rect)
{
	setGeometry(rect);
	ui.ChatListPopUpWidget->setFixedHeight(rect.height()-1);
	ui.ChatListPopUpWidget->setFixedWidth(rect.width());
}

void ChatListPopUpWidget::itemClicked(QListWidgetItem* item)
{
	if (m_itemParentWidget) {
		m_itemParentWidget->SetBtnText(item->text());
		m_itemParentWidget->ResetSelect(m_itemParentWidget->GetSelected());
	}

	m_isItemSelected = true;

	m_currentRow = ui.ChatListPopUpWidget->row(item);

	hide();
}

void ChatListPopUpWidget::show()
{
	m_isItemSelected = false;

	QFramelessPopup::show();
}

void ChatListPopUpWidget::hide()
{
	if (m_isItemSelected)
	{

	}
	else
	{
		ui.ChatListPopUpWidget->setCurrentRow(m_savedRow);
	}

	QFramelessPopup::hide();
}

void ChatListPopUpWidget::SaveCurrentRow()
{
	m_savedRow = ui.ChatListPopUpWidget->currentRow();
}

void ChatListPopUpWidget::SetRoomInfo(vector<ChatRoomInfo*>& roomInfo)
{
	for (int i = 0; i < roomInfo.size(); i++) {
		ui.ChatListPopUpWidget->addItem(roomInfo[i]->chatRoomName);
		m_vecRoomName.push_back(roomInfo[i]->chatRoomName);
	}

	ui.ChatListPopUpWidget->setCurrentRow(0);
}

void ChatListPopUpWidget::ChangeIconColor()
{
	if (m_itemParentWidget)
	{
		const vector<ChatRoomInfo*>& tempVec = m_itemParentWidget->GetRoomInfo();

		for (int i = 0; i < tempVec.size(); i++)
		{
			QList<QListWidgetItem *> items = ui.ChatListPopUpWidget->findItems(tempVec[i]->chatRoomName, Qt::MatchExactly);
			ui.ChatListPopUpWidget->setIconSize(QSize(CHAT_POPUP_ICON_SIZE, CHAT_POPUP_ICON_SIZE));

			QPixmap pm;
			if (tempVec[i]->bVoiceRoom)
			{
				pm = m_headsetImage;
			}
			else
			{
				pm = m_chatImage;
			}

			QIcon icon;
			icon.addPixmap(pm, QIcon::Normal);
			icon.addPixmap(pm, QIcon::Selected);

			items[items.size() - 1]->setIcon(icon);
		}
	}
}

int ChatListPopUpWidget::GetMaximumWidth()
{
	int maxRoomNameWidth = 0;
	QFont roomNameFont = AppUtils::GetRegularFont(CHAT_NAME_LABEL_FONT_SIZE);
	QFontMetrics fm(roomNameFont);

	for (int i = 0; i < ui.ChatListPopUpWidget->count(); i++) {
		int temp_maxRoomNameWidth = fm.horizontalAdvance(m_vecRoomName[i]);
		if (maxRoomNameWidth < temp_maxRoomNameWidth) maxRoomNameWidth = temp_maxRoomNameWidth;
	}

	return maxRoomNameWidth;
}