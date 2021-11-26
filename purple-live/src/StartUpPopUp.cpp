#include "StartUpPopUp.h"

#include "ScreenLockWidget.h"
#include "MainWindow.h"
#include "AppString.h"

#include <qpainter.h>

StartUpPopUp::StartUpPopUp(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	InitUI();

	connect(&m_timer, &QTimer::timeout, this, &StartUpPopUp::Timeout);
	m_timer.setInterval(FADEOUT_TIMER_INTERVAL);

	ui.verticalWidget->installEventFilter(this);

	connect(MainWindow::Get()->GetStartupWindow(), &StartUpWindow::ResetGeometry, this, &StartUpPopUp::ResetGeometry);
	connect(MainWindow::Get(), &MainWindow::UpdateChildLanguageString, this, &StartUpPopUp::UpdateLanguageString);
}

StartUpPopUp::~StartUpPopUp()
{
}

void StartUpPopUp::InitUI()
{
	setAttribute(Qt::WA_TranslucentBackground);

	ui.verticalWidget->style()->unpolish(ui.verticalWidget);
	ui.verticalWidget->style()->polish(ui.verticalWidget);

	hide();
}

void StartUpPopUp::SetTextCancelBtn(QString text)
{
	ui.cancel_btn->setText(text);
}

void StartUpPopUp::SetTextSelectBtn(QString text)
{
	ui.select_btn->setText(text);
}

void StartUpPopUp::SetShowButton(bool show)
{
	m_showButton = show;
	SetHideButtonWidget();
}

void StartUpPopUp::SetChildWidget(QWidget* w)
{
	ui.childWidget_layout->addWidget(w);
	m_child = w;
	m_child->setParent(this);
	ui.select_btn->setParent(ui.buttonWidget);
	ui.cancel_btn->setParent(ui.buttonWidget);
}

int StartUpPopUp::GetStartUpPopUpWidth()
{
	return MainWindow::Get()->GetStartupWindow()->width() - STARTPOPUP_X_MARGIN * 2;
}

QRect StartUpPopUp::GetStartUpPopUpRect()
{
	return QRect(STARTPOPUP_X_MARGIN, GetStartUpPopUpY(), GetStartUpPopUpWidth(), GetStartUpPopUpHeight());
}

void StartUpPopUp::CaptureCurrentScreen()
{
	HideAllItems(false);

	SetGeoMetry(GetStartUpPopUpRect());

	// Set Child Height
	if (m_child)
	{
		m_child->setFixedHeight(GetStartUpPopUpHeight() - (m_showButton ? STARTPOPUP_CHILD_TOP_BOTTOM_MARGIN : STARTPOPUP_CHILD_TOP_MARGIN));
	}

	// Capture Screen
	m_screenCapture = QPixmap(GetStartUpPopUpWidth(), GetStartUpPopUpHeight());
	if (m_screenCapture.isNull()) {
		// if get widget size fail
	}
	else
	{
		this->render(&m_screenCapture, QPoint(), QRegion(QRect(0, 0, GetStartUpPopUpWidth(),GetStartUpPopUpHeight())));
	}

	HideAllItems(true);
}

void StartUpPopUp::TransparentMainWidget(bool transparent)
{
	if (transparent)
	{
		// transparent
		ui.verticalWidget->setProperty("Style", "1");
	}
	else
	{
		ui.verticalWidget->setProperty("Style", "0");
	}

	ui.verticalWidget->style()->unpolish(ui.verticalWidget);
	ui.verticalWidget->style()->polish(ui.verticalWidget);
}

void StartUpPopUp::ActiveStartUpPopUp(bool bshow)
{
	if (m_timer.isActive()) return;
	m_timer.start();
	m_drawCnt = FADEOUT_DRAW_CNT;

	m_bHide = !bshow;
	SetGeoMetry(GetStartUpPopUpRect());

	if (bshow)
	{
		// show
		ShowDimWidget(true);
		QWidget::show();
		QWidget::raise();
	}
	else
	{
		// hide
	}

	CaptureCurrentScreen();
	TransparentMainWidget(true);
}

void StartUpPopUp::showEvent(QShowEvent* e)
{
	connect(ScreenLockWidget::Instance(), &ScreenLockWidget::SendScreenLockShowEvent, this, &StartUpPopUp::OnScreenLockShowEvent);
	connect(ScreenLockWidget::Instance(), &ScreenLockWidget::SendScreenLockHidedEvent, this, &StartUpPopUp::OnScreenLockHideEvent);
}

void StartUpPopUp::ResetGeometry()
{
	SetGeoMetry(GetStartUpPopUpRect());
}

void StartUpPopUp::DrawImage(QPixmap* pixmap, QPainter* painter, int _x, int _y, int _width, int _height)
{
	QPixmap scaled = pixmap->scaled(_width, _height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	QImage fixedImage(_width, _height, QImage::Format_ARGB32_Premultiplied);
	fixedImage.fill(0);
	QPainter imgPainter(&fixedImage);
	QPainterPath clip;
	clip.addRoundedRect(QRect(0, 0, _width, _height), 9, 9);
	imgPainter.setClipPath(clip);
	imgPainter.drawPixmap(0, 0, _width, _height, scaled);
	imgPainter.end();

	painter->drawPixmap(_x, _y, _width, _height, QPixmap::fromImage(fixedImage));
}

void StartUpPopUp::show()
{
	ActiveStartUpPopUp(true);
}

void StartUpPopUp::hide()
{
	if (m_bHide) {
		QWidget::hide();
		return;
	}
	HideEventCalled();
	ActiveStartUpPopUp(false);
}

void StartUpPopUp::ShowDimWidget(bool bshow)
{
	if(bshow) MainWindow::Get()->GetStartupWindow()->ShowLoading(this);
	else MainWindow::Get()->GetStartupWindow()->HideLoading();
}

void StartUpPopUp::SetGeoMetry(QRect& _r)
{
	// Set Widget Geo
	setFixedWidth(_r.width());
	setFixedHeight(_r.height());
	ui.verticalWidget->setFixedWidth(_r.width());
	ui.verticalWidget->setFixedHeight(_r.height());
	setGeometry(_r.x(), _r.y(), _r.width(), _r.height());

	// Set Button Geo
	QRect r;
	// slide up
	r = ui.childWidget_layout->geometry();
	m_child->setGeometry(r.x(),
		STARTPOPUP_CHILD_TOP_MARGIN,
		width(),
		m_child->height()
	);
}

bool StartUpPopUp::eventFilter(QObject* obj, QEvent* event)
{
	if (obj == ui.verticalWidget)
	{
		if (event->type() == QEvent::Paint)
		{
			QPainter painter(ui.verticalWidget);

			if (m_screenCapture.isNull() == false && m_timer.isActive())
			{
				if (!m_bHide)
				{
					// show
					if (!(--m_drawCnt))
						StopFading(true);

					float opacity = 1.0 - float(m_drawCnt) / FADEOUT_DRAW_CNT;
					painter.setOpacity(opacity);
				}
				else
				{
					// hide
					if (!(--m_drawCnt))
						StopFading(false);

					float opacity = float(m_drawCnt) / FADEOUT_DRAW_CNT;
					painter.setOpacity(opacity);
				}

				DrawImage(&m_screenCapture, &painter, 
					0,
					0,
					m_screenCapture.width(), 
					m_screenCapture.height());
				
			}

			painter.end();
		}
	}

	return QWidget::eventFilter(obj, event);
}

void StartUpPopUp::StopFading(bool show)
{
	m_timer.stop();
	m_drawCnt = 0;
	TransparentMainWidget(false);

	if (show)
	{
		// show event : stop fading
		HideAllItems(false);
		update();
	}
	else
	{
		// hide event : stop fading
		ShowDimWidget(false);
		QWidget::hide();
	}
}

void StartUpPopUp::Timeout()
{
	update();
}

void StartUpPopUp::HideAllItems(bool hide)
{
	if (hide)
	{
		ui.slideupTitle_label->hide();
		if (m_child) m_child->hide();
		ui.cancel_btn->hide();
		ui.select_btn->hide();
		ui.popup_close_btn->hide();
	}
	else
	{
		ui.slideupTitle_label->show();
		if (m_child) m_child->show();
		ui.cancel_btn->show();
		ui.select_btn->show();
		ui.popup_close_btn->show();
	}
}

void StartUpPopUp::UpdateLanguageString()
{
	ui.cancel_btn->setText(AppString::GetQString("Common/cancel"));
	ui.select_btn->setText(AppString::GetQString("SlideUpDlg/select"));
	UpdateLanguageStringCalled();
}

void StartUpPopUp::OnScreenLockShowEvent()
{
	if (isVisible())
	{
		m_pastIsVisible = true;
		QWidget::hide();
		ShowDimWidget(false);
	}
}

void StartUpPopUp::OnScreenLockHideEvent()
{
	if (m_pastIsVisible)
	{
		m_pastIsVisible = false;
		QWidget::show();
		ShowDimWidget(true);
	}
}
