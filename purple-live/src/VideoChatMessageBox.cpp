#include "VideoChatMessageBox.h"

#include "AppUtils.h"
#include "AppString.h"
#include "MainWindow.h"
#include <qapplication.h>
#include <qsettings.h>

VideoChatMessageBox::VideoChatMessageBox(QWidget *parent)
	: QFramelessPopup(parent)
{
	ui.setupUi(this);

	setObjectName("VideoChatMessageBox");
	connect(&m_timer, SIGNAL(timeout()), this, SLOT(OnTimerCallbackFunc()));
	connect(MainWindow::Get()->GetStartupWindow(), &StartUpWindow::ResetGeometry, this, &VideoChatMessageBox::ResetGeometry);

	InitUI();
}

VideoChatMessageBox::~VideoChatMessageBox()
{
}

void VideoChatMessageBox::ResetGeometry()
{
	if (m_parentHide) {
		QRect default_rect = AppUtils::GetScreenCenterGeometry(this, m_currentWidth, m_currentHeight);
		SetGeometry(default_rect);
	}
	else {
		QRect newRect = { m_parent->x() + m_parent->width() / 2 - m_currentWidth / 2,
			m_parent->y() + m_parent->height() / 2 - m_currentHeight / 2,
			m_currentWidth,
			m_currentHeight };

		SetGeometry(newRect);
	}
}

void VideoChatMessageBox::SetGeometry(const QRect& rect)
{
	setGeometry(rect);
	setFixedWidth(rect.width() + SHADOW_X_MARGIN);
	setFixedHeight(rect.height() + SHADOW_Y_MARGIN);
	ui.main_widget->setFixedWidth(rect.width());
	ui.main_widget->setFixedHeight(rect.height());
}

bool VideoChatMessageBox::question(const std::string &text)
{
	ui.cancelWidget->show();

	m_parent = MainWindow::GetActiveWidget();
	m_InformMainMessage = QString::fromUtf8(text.c_str());

	if (ScreenLockWidget::Instance()->IsActive())
	{
		connect(ScreenLockWidget::Instance(), &ScreenLockWidget::SendScreenLockHidedEvent, this, &VideoChatMessageBox::exec);
	}
	else
	{
		exec();
	}

	QPointer<QEventLoop> loop = new QEventLoop;
	m_loop = loop;
	connect(ui.ok_btn, &QPushButton::clicked, m_loop, &QEventLoop::quit);
	connect(ui.close_btn, &QPushButton::clicked, m_loop, &QEventLoop::quit);
	loop->exec();

	return m_answer;
}

void VideoChatMessageBox::OnScreenLockShowEvent()
{
	if (m_loop.isNull() == false)
	{
		SetDimWidget(false);
		hide();
	}
}

void VideoChatMessageBox::OnScreenLockHideEvent()
{
	if (m_loop.isNull() == false)
	{
		ResetGeometry();
		SetDimWidget(true);
		show();
	}
}

bool VideoChatMessageBox::selection(const std::string &text, const std::string &select1, const std::string &select2)
{
	ui.cancelWidget->show();

	m_parent = MainWindow::GetActiveWidget();
	m_InformMainMessage = QString::fromUtf8(text.c_str());

	if (ScreenLockWidget::Instance()->IsActive())
	{
		connect(ScreenLockWidget::Instance(), &ScreenLockWidget::SendScreenLockHidedEvent, this, &VideoChatMessageBox::exec);
	}
	else
	{
		exec();
	}

	QPointer<QEventLoop> loop = new QEventLoop;
	m_loop = loop;
	ui.ok_btn->setText(QString::fromUtf8(select1.c_str()));
	ui.cancel_btn->setText(QString::fromUtf8(select2.c_str()));
	connect(ui.ok_btn, &QPushButton::clicked, loop, &QEventLoop::quit);
	connect(ui.close_btn, &QPushButton::clicked, loop, &QEventLoop::quit);
	connect(ui.cancel_btn, &QPushButton::clicked, loop, &QEventLoop::quit);
	loop->exec();

	return m_answer;
}

void VideoChatMessageBox::information(const std::string &mainText, const std::string &subText)
{ 
	ui.cancelWidget->hide();
	information_stimer(mainText, subText, -1);
}

int VideoChatMessageBox::SetInformLabelMessage(QString message, bool isMainMessage)
{
	QString mainMessage = message;
	QString labelObjectName = isMainMessage ? "MainMessage" : "SubMessage";
	int newLineIndex = 0;
	int newLineIndexCnt = 0;
	int max_subTextLength = 0;
	QString maximumWidthString = 0;

	// insert Text
	while ((newLineIndexCnt = mainMessage.indexOf('\n', newLineIndexCnt)) != -1) {
		QString _text = mainMessage.mid(newLineIndex, newLineIndexCnt - newLineIndex);
		QLabel *label = new QLabel(ui.textMessage_widget);
		label->setObjectName(labelObjectName);
		label->setText(_text);
		label->setAlignment(Qt::AlignHCenter);
		if(isMainMessage) m_vecInformMessage.push_back(label);
		else m_vecInformSubMessage.push_back(label);

		if (_text.length() > max_subTextLength) {
			max_subTextLength = _text.length();
			maximumWidthString = _text;
		}
		newLineIndex = ++newLineIndexCnt;
	}

	QString _text = mainMessage.mid(newLineIndex, mainMessage.size() - newLineIndex);
	QLabel *label = new QLabel(ui.textMessage_widget);
	label->setObjectName(labelObjectName);
	label->setText(_text);
	label->setAlignment(Qt::AlignHCenter);
	if (isMainMessage) m_vecInformMessage.push_back(label);
	else m_vecInformSubMessage.push_back(label);

	if (_text.length() > max_subTextLength) {
		max_subTextLength = _text.length();
		maximumWidthString = _text;
	}
	
	QFont textFont = isMainMessage ? AppUtils::GetMediumFont(13) : AppUtils::GetRegularFont(12);
	
	return DrawUtils::GetFontWidth(textFont, maximumWidthString);
}

void VideoChatMessageBox::exec()
{
	SetDimWidget(true);

	m_vecInformMessage.clear();
	m_vecInformSubMessage.clear();

	int maxFontWidth = SetInformLabelMessage(m_InformMainMessage);
	
	if (m_InformSubMessage.isEmpty() == false)
	{
		int maxSubFontWidth = SetInformLabelMessage(m_InformSubMessage, false);
		if (maxSubFontWidth > maxFontWidth) maxFontWidth = maxSubFontWidth;
	}

	int new_width = MSG_BOX_MIN_WIDTH;
	int new_height = MSG_BOX_TEXT_MAIN_HEIGHT * m_vecInformMessage.size() + MSG_BOX_TEXT_SUB_HEIGHT * m_vecInformSubMessage.size() + MSG_BOX_TOP_MARGIN + MSG_BOX_BOTTOM_MARGIN;

	// check minimum width
	if (maxFontWidth <= MSG_BOX_MIN_WIDTH - WIDTH_PADDING) {}
	else { new_width = maxFontWidth + WIDTH_PADDING; }

	// check minimum height
	if (new_height <= MSG_BOX_MIN_HEIGHT)
		new_height = MSG_BOX_MIN_HEIGHT;

	int i = 0;
	for (; i < m_vecInformMessage.size(); i++) {
		m_vecInformMessage[i]->setGeometry(0, MSG_BOX_TEXT_MAIN_HEIGHT * i, new_width, MSG_BOX_TEXT_MAIN_HEIGHT);
		m_vecInformMessage[i]->show();
	}

	for (int j = 0; j < m_vecInformSubMessage.size(); j++) {
		m_vecInformSubMessage[j]->setGeometry(0, MSG_BOX_TEXT_MAIN_HEIGHT * i + MSG_BOX_TEXT_SUB_HEIGHT * j, new_width, MSG_BOX_TEXT_SUB_HEIGHT);
		m_vecInformSubMessage[j]->show();
	}

	ui.textMessage_widget->setFixedWidth(new_width);
	ui.textMessage_widget->setFixedHeight(MSG_BOX_TEXT_MAIN_HEIGHT * (m_vecInformMessage.size() + 1) + MSG_BOX_TEXT_SUB_HEIGHT * m_vecInformSubMessage.size());
	
	m_currentWidth = new_width;
	m_currentHeight = new_height;

	if (m_parentHide) {
		QRect default_rect = AppUtils::GetScreenCenterGeometry(this, new_width, new_height);
		SetGeometry(default_rect);
	}
	else {
		QRect newRect = { m_parent->x() + m_parent->width() / 2 - new_width / 2,
			m_parent->y() + m_parent->height() / 2 - new_height / 2,
			new_width,
			new_height };

		SetGeometry(newRect);
	}

	show();
}

void VideoChatMessageBox::on_close_btn_clicked()
{
	SetDimWidget(false);

	for (int i = 0; i < m_vecInformMessage.size(); i++)
		m_vecInformMessage[i]->hide();

	m_vecInformMessage.clear();
	m_vecInformSubMessage.clear();
	m_timer.stop();

	hide();

	if (MainWindow::Get()->getStartUpWindowIsHidden() == false)
	{
		MainWindow::Get()->GetStartupWindow()->HideLoading();

		if (MainWindow::Get()->GetStartupWindow()->IsDimmingShow())
		{
			MainWindow::Get()->GetStartupWindow()->ShowDimmingIndicator();
		}
	}

	deleteLater();
}

void VideoChatMessageBox::on_ok_btn_clicked()
{
	on_close_btn_clicked();
	m_answer = true;
}

void VideoChatMessageBox::on_cancel_btn_clicked()
{
	on_close_btn_clicked();
	m_answer = false;
}

void VideoChatMessageBox::information_stimer(const std::string &mainText, const std::string &subText, int _time)
{
	m_parent = MainWindow::GetActiveWidget();
	QString s = QString::fromUtf8(mainText.c_str());
	if (_time > 0) {
		m_timerData.text = s;
		m_timerData.time = _time;
		s.replace(QString("<stime>"), QString::number(_time));
		m_timer.start(1000);
	}

	m_InformMainMessage = s;
	m_InformSubMessage = QString::fromUtf8(subText.c_str());

	if (ScreenLockWidget::Instance()->IsActive())
	{
		connect(ScreenLockWidget::Instance(), &ScreenLockWidget::SendScreenLockHidedEvent, this, &VideoChatMessageBox::exec);
	}
	else
	{
		exec();
	}

	QPointer<QEventLoop> loop = new QEventLoop;
	m_loop = loop;
	connect(ui.ok_btn, &QPushButton::clicked, loop, &QEventLoop::quit);
	connect(ui.close_btn, &QPushButton::clicked, loop, &QEventLoop::quit);
	loop->exec();
}

void VideoChatMessageBox::SetDimWidget(bool enable)
{
	if (enable)
	{
		if (MainWindow::Get()->isHidden() && MainWindow::Get()->getStartUpWindowIsHidden()) 
		{
			// 모든 윈도우가 없을 시 dimming은 나타내지 않는다.		
			m_parentHide = true;
		}
		else {
			// MainWindow Dimming
			if (MainWindow::Get()->isHidden() == false) {
				// MainWindow일 경우 resize를 허용
				if (!m_dimWidget)
				{
					m_dimWidget = make_unique<dimmingWidget>(m_parent);
				}
				m_dimWidget->setCurrentGeo(m_parent);
				connect(m_dimWidget.get(), &dimmingWidget::ResetChildGeometry, this, &VideoChatMessageBox::ResetGeometry);
				connect(MainWindow::Get(), &MainWindow::OnActiveWindow, m_dimWidget.get(), &dimmingWidget::RaiseChildWidget);

				m_dimWidget->SetEnableResize(true);
				m_dimWidget->SetChild(this);
				m_dimWidget->show();
			}

			// StartUpWindow Dimming
			if (MainWindow::Get()->getStartUpWindowIsHidden() == false)
			{	
				MainWindow::Get()->GetStartupWindow()->ShowLoading(this);
			}
			
			m_parentHide = false;
		}
	}
	else
	{
		if (MainWindow::Get()->getStartUpWindowIsHidden() == false) {
			MainWindow::Get()->GetStartupWindow()->DisableAllButtons(false);
			MainWindow::Get()->GetStartupWindow()->HideLoading();
		}
		else {
			if (m_dimWidget)
			{
				m_dimWidget->hide();
			}
		}
	}
}

void VideoChatMessageBox::OnTimerCallbackFunc()
{
	if (--m_timerData.time > 0)
	{
		QString s = m_timerData.text;
		s.replace(QString("<stime>"), QString::number(m_timerData.time));
		m_InformMainMessage = s;
		ChangeInformText();
	}
	else
	{
		m_timer.stop();
		ui.close_btn->click();
	}
}

void VideoChatMessageBox::ChangeInformText()
{
	for (int i = 0; i < m_vecInformMessage.size(); i++)
		m_vecInformMessage[i]->hide();


	m_vecInformMessage.clear();

	QString s = m_InformMainMessage;
	int newLineCnt = s.count('\n');

	int newLineIndex = 0;
	int newLineIndexCnt = 0;
	QString maximumWidthString = 0;
	int max_subTextLength = 0;

	auto insertLabelText = [&]() {
		QString subText = s.mid(newLineIndex, newLineIndexCnt - newLineIndex);
		QLabel *label = new QLabel(ui.textMessage_widget);
		label->setObjectName("MainMessage");
		label->setText(subText);
		label->setAlignment(Qt::AlignHCenter);
		m_vecInformMessage.push_back(label);

		if (subText.length() > max_subTextLength) {
			max_subTextLength = subText.length();
			maximumWidthString = subText;
		}
		newLineIndex = ++newLineIndexCnt;
	};

	// insert Label Text
	while ((newLineIndexCnt = s.indexOf('\n', newLineIndexCnt)) != -1) {
		insertLabelText();
	}

	insertLabelText();

	// calculate new width
	int maxFontWidth = DrawUtils::GetFontWidth(AppUtils::GetMediumFont(13), maximumWidthString);
	int new_width = MSG_BOX_MIN_WIDTH;
	if (maxFontWidth <= MSG_BOX_MIN_WIDTH - WIDTH_PADDING) {}
	else { new_width = maxFontWidth + WIDTH_PADDING; }

	for (int i = 0; i < m_vecInformMessage.size(); i++) {
		m_vecInformMessage[i]->setGeometry(0, MSG_BOX_TEXT_MAIN_HEIGHT * i, new_width, MSG_BOX_TEXT_MAIN_HEIGHT);
		m_vecInformMessage[i]->show();
	}
}

void VideoChatMessageBox::InitUI()
{
	setWindowFlags(windowFlags() | Qt::Tool);
	setWindowModality(Qt::WindowModality::NonModal);
	setAttribute(Qt::WA_TranslucentBackground);

	ui.ok_btn->setText(AppString::GetQString("Common/confirm"));
	ui.cancel_btn->setText(AppString::GetQString("Common/cancel"));
	ui.close_btn->SetIconSize(MSG_BOX_CLOSE_BTN_ICON_SIZE, MSG_BOX_CLOSE_BTN_ICON_SIZE);

	QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect(ui.main_widget);
	effect->setBlurRadius(10);
	effect->setColor(SHADOW_COLOR);
	effect->setOffset(QPointF(0, 10));
	ui.main_widget->setGraphicsEffect(effect);
}