#include "ViewerListItem.h"

#include "MainWindow.h"
#include "AppUtils.h"
#include "AppString.h"
#include "VideoChatMessageBox.h"

#include <qpainter.h>

ViewerListItem::ViewerListItem(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	m_userEmptyProfile = QPixmap(":/StartStreamDlg/image/ic_default_character.png");
	m_deportBtnPixmap = QPixmap(":/MainWindow/image/ic_deport_btn.png");
	ui.deport_btn->installEventFilter(this);
}

ViewerListItem::~ViewerListItem()
{
}

void ViewerListItem::SetUserInfo(StreamRoomUserInfo& _info)
{
	m_roomUserInfo = _info;

	if (m_roomUserInfo.IsGuestRole())
	{
		ui.deport_btn->show();
		ui.viewerName_label->setProperty("isGuest", "1");
		ui.viewerName_label->style()->unpolish(ui.viewerName_label);
		ui.viewerName_label->style()->polish(ui.viewerName_label);

	}
	else
	{
		ui.deport_btn->hide();
	}

	QString name = QString::fromStdString(m_roomUserInfo.name);

	if (DrawUtils::GetFontWidth(AppUtils::GetMediumFont(13), name) > VIEWER_NAME_MAX_WIDTH)
	{
		for (int i = name.size()-1; ; i--)
		{
			QString newName = name.mid(0, i);

			if (DrawUtils::GetFontWidth(AppUtils::GetMediumFont(13), newName) <= VIEWER_NAME_MAX_WIDTH)
			{
				newName += "...";
				name = newName;
				break;
			}
		}

		ui.viewerName_label->setFixedWidth(DrawUtils::GetFontWidth(AppUtils::GetMediumFont(13), name));
	}

	ui.viewerName_label->setText(name);
}

void ViewerListItem::SetIconColor(QColor val)
{
	m_iconColor = val;
	m_deportBtnPixmap = DrawUtils::ChangeIconColor(m_deportBtnPixmap, m_iconColor);
}

void ViewerListItem::paintEvent(QPaintEvent *e)
{
	QPainter painter(this);

	QRect _profileR(16,								// x
		height() / 2 - VIEWER_IMAGE_SIZE / 2,		// y
		VIEWER_IMAGE_SIZE,							// width
		VIEWER_IMAGE_SIZE);							// height

	if (m_userProfile.isNull() == false)
	{
		DrawUtils::DrawEllipseImage(&m_userProfile, &painter, _profileR);
	}
	else
	{
		DrawUtils::DrawEllipseImage(&m_userEmptyProfile, &painter, _profileR);
	}

	painter.end();
}

void ViewerListItem::on_deport_btn_clicked()
{
	if (VideoChatMessageBox::Instance()->question(AppUtils::format(AppString::GetString("MainDlg/deport_check_message").c_str(), m_roomUserInfo.name.c_str())))
	{
		DeportUserInfo info{ m_roomUserInfo.gameUserId, true };
		MainWindow::Get()->RequestDeportRoom(info);
	}
	else {
		// ¹«½Ã
	}
}

bool ViewerListItem::eventFilter(QObject* obj, QEvent* event)
{
	if (ui.deport_btn == obj)
	{
		if (event->type() == QEvent::Paint)
		{
			if (ui.deport_btn->isHidden() == false &&
				m_deportBtnPixmap.isNull() == false)
			{
				QPainter painter(ui.deport_btn);
				int _x = ui.deport_btn->width() / 2 - VIEWER_DEPORT_IMAGE_SIZE / 2;
				int _y = ui.deport_btn->height() / 2 - VIEWER_DEPORT_IMAGE_SIZE / 2;
				DrawUtils::DrawRectImage(&m_deportBtnPixmap,&painter,
					QRect(_x, _y, VIEWER_DEPORT_IMAGE_SIZE, VIEWER_DEPORT_IMAGE_SIZE));
			}
		}
	}

	return QWidget::eventFilter(obj, event);;
}