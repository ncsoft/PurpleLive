#include "OpenLicensePopup.h"

#include <Windows.h>
#include <qtextdocument.h>
#include <qmenu.h>

#include "MainWindow.h"
#include "ScreenLockWidget.h"
#include "VideoChatApp.h"
#include "AppConfig.h"
#include "AppString.h"

OpenLicensePopup::OpenLicensePopup(QWidget *parent)
	: QFramelessDialog(parent, true, 0, false), m_parent(parent)
{
	ui.setupUi(this);

	InitUI();
}

OpenLicensePopup::~OpenLicensePopup()
{
}

void OpenLicensePopup::InitUI()
{
	setAttribute(Qt::WA_TranslucentBackground);
	setWindowModality(Qt::WindowModality::NonModal);
	ui.licensePolicyTextEdit->setReadOnly(true);

	UpdateLanguageString();

	ui.licensePolicyTextEdit->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui.licensePolicyTextEdit, SIGNAL(customContextMenuRequest(const QPoint&)), this, SLOT(showContextMenu(const QPoint &)));

	QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect(ui.centralwidget);
	effect->setBlurRadius(10);
	effect->setColor(SHADOW_COLOR);
	effect->setOffset(QPointF(0, 10));
	ui.centralwidget->setGraphicsEffect(effect);

	QObject::connect(MainWindow::Get(), &MainWindow::UpdateChildLanguageString, this, &OpenLicensePopup::UpdateLanguageString);
}

void OpenLicensePopup::UpdateLanguageString()
{
	ui.ok_btn->setText(AppString::GetQString("Common/confirm"));
	ui.purpleLive_label->setText(AppString::GetQString("openlicensepopup/purplelive_title"));
	ui.ncSoft_label->setText(AppString::GetQString("openlicensepopup/ncsoft_name"));
	ui.verSion_label->setText(GetAppVersion());
	ui.aboutInfo_label->setText(AppString::GetQString("openlicensepopup/about_info"));
	ui.aboutInfo_label->setWordWrap(true);
}

void OpenLicensePopup::showContextMenu(const QPoint &pt)
{
	// 마우스 우클릭으로 나타나는 ContextMenu를 막는다
	QMenu *menu = ui.licensePolicyTextEdit->createStandardContextMenu();
	menu->exec(ui.licensePolicyTextEdit->mapToGlobal(pt));
	delete menu;
}

void OpenLicensePopup::Init()
{
	ResetGeometry();

	connect(ScreenLockWidget::Instance(), &ScreenLockWidget::SendScreenLockShowEvent, this, &OpenLicensePopup::OnScreenLockShowEvent);
	connect(ScreenLockWidget::Instance(), &ScreenLockWidget::SendScreenLockHidedEvent, this, &OpenLicensePopup::OnScreenLockHideEvent);
}

void OpenLicensePopup::Show()
{
	UpdateLicenseText();

	SetDimWidget(true);

	show();
}

void OpenLicensePopup::OnScreenLockShowEvent()
{
	if (!m_dimWidget.isNull())
	{
		m_dimWidget->hide();
		hide();
	}
}

void OpenLicensePopup::OnScreenLockHideEvent()
{
	ResetGeometry();
	SetDimWidget(true);
	show();
}

void OpenLicensePopup::UpdateLicenseText()
{
	QNetworkAccessManager *manager = new QNetworkAccessManager(this);
	connect(manager, &QNetworkAccessManager::finished, this, &OpenLicensePopup::LicenseTextDownFinished);
	connect(manager, &QNetworkAccessManager::finished, manager, &QNetworkAccessManager::deleteLater);

	QUrl url;

	switch (App()->GetServiceNetwork())
	{
	case eServiceNetwork::eSN_RC:
		url.setUrl(AppConfig::Instance()->OPEN_SOURCE_POLICY_URL_RC);
		break;
	default:
		url.setUrl(AppConfig::Instance()->OPEN_SOURCE_POLICY_URL_LIVE);
		break;
	}

	QNetworkRequest request(url);
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

	QNetworkReply* reply = manager->get(request);
}

void OpenLicensePopup::LicenseTextDownFinished(QNetworkReply *reply)
{
	QByteArray qbytes = reply->readAll();
	QString strHtml(qbytes);

	QTextDocument document;
	document.setHtml(strHtml);
	QString str = document.toPlainText();

	ui.licensePolicyTextEdit->setText(str);
}

void OpenLicensePopup::on_openLisencePopCloseBtn_clicked()
{
	SetDimWidget(false);
	close();
	deleteLater();
}

void OpenLicensePopup::on_ok_btn_clicked()
{
	on_openLisencePopCloseBtn_clicked();
}

QString OpenLicensePopup::GetAppVersion()
{
	QString result = QString::fromStdString(App()->GetAppVersion());

	if (sizeof(void *) == 4)
		result += "(32 bit)";
	else if (sizeof(void *) == 8)
		result += "(64 bit)";

	return result;
}

void OpenLicensePopup::SetDimWidget(bool enable)
{
	if (enable)
	{
		m_dimWidget = new dimmingWidget(m_parent);
		connect(m_dimWidget, &dimmingWidget::ResetChildGeometry, this, &OpenLicensePopup::ResetGeometry);
		m_dimWidget->setCurrentGeo(m_parent);
		if (MainWindow::Get()->isHidden()) {
			// parent is hidden
			if (m_dimWidget)
			{
				m_dimWidget->hide();
				m_dimWidget->deleteLater();
			}
		}
		else {
			m_dimWidget->SetChild(this);
			m_dimWidget->SetEnableResize(true);
			m_dimWidget->show();
		}
	}
	else
	{
		if (m_dimWidget)
		{
			m_dimWidget->hide();
			m_dimWidget->deleteLater();
		}
	}
}

void OpenLicensePopup::SetFixedSize(int _width, int _height)
{
	ui.centralwidget->setFixedWidth(_width);
	ui.centralwidget->setFixedHeight(_height);
	setFixedWidth(_width + SHADOW_PADDING_X);
	setFixedHeight(_height + SHADOW_PADDING_Y);
}

void OpenLicensePopup::ResetGeometry()
{
	QWidget* parentWidget = static_cast<QWidget*>(parent());

	int newW = parentWidget->width() - POPUP_XY_MARGIN * 2;
	int newH = parentWidget->height() - POPUP_XY_MARGIN * 2;

	if (newW < LICENSEPOPUP_MIN_WIDTH)
	{
		newW = LICENSEPOPUP_MIN_WIDTH;
	}
	else if (newW > LICENSEPOPUP_MAX_WIDTH)
	{
		newW = LICENSEPOPUP_MAX_WIDTH;
	}

	if (newH < LICENSEPOPUP_MIN_HEIGHT)
	{
		newH = LICENSEPOPUP_MIN_HEIGHT;
	}
	else if (newH > LICENSEPOPUP_MAX_HEIGHT)
	{
		newH = LICENSEPOPUP_MAX_HEIGHT;
	}

	SetFixedSize(newW, newH);

	move(parentWidget->x() + parentWidget->width() / 2 - newW / 2,
		parentWidget->y() + parentWidget->height() / 2 - newH / 2);
}
