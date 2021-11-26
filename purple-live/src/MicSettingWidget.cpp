#include "MicSettingWidget.h"

#include "MainWindow.h"
#include "LimeCommunicator.h"
#include "VideoChatApp.h"
#include "AppString.h"
#include "AppUtils.h"
#include <LocalSettings.h>

#include <qevent.h>
#include <qpainter.h>

MicSettingWidget::MicSettingWidget(QWidget *parent)
	: StartUpPopUp(parent)
{
	ui.setupUi(this);

	SetChildWidget(ui.mainWidget);

	connect(ui.mic_listWidget, QOverload<int>::of(&QListWidget::currentRowChanged), this, QOverload<int>::of(&MicSettingWidget::ItemRowChanged));
	connect(ui.micVolume_slider, QOverload<int>::of(&QSlider::valueChanged), this, QOverload<int>::of(&MicSettingWidget::SliderValueChanged));

	ui.micVolume_slider->installEventFilter(this);
	float MicVolume{ 0.0 };
	MicVolume = LocalSettings::GetMicVolume(MicVolume) * 100.0;
	ui.micVolume_slider->setTickInterval(1);
	ui.slider_widget->installEventFilter(this);

	m_initWidget = true;
	ui.micVolume_slider->setValue(int(MicVolume));
}

MicSettingWidget::~MicSettingWidget()
{
}

void MicSettingWidget::SliderValueChanged(int value)
{
	if (!m_initWidget) return;
	bool micCheckBoxChecked = ui.mute_checkbox->isChecked();

	if (micCheckBoxChecked)
	{
		// Muted Checked
		if (value != ui.micVolume_slider->minimum()) {
			ui.mute_checkbox->setChecked(false);
		}
	}
	else
	{
		// Muted UnChecked
		if (value == ui.micVolume_slider->minimum()) {
			ui.mute_checkbox->setChecked(true);
		}
	}
}

void MicSettingWidget::show()
{
	m_lastMicVolume = LocalSettings::GetMicVolume(0.0) * 100.0;

	if (m_selectedIndex < 0) m_selectedIndex = 0;
	ui.mic_listWidget->setCurrentRow(m_selectedIndex);
	m_lastSelectedIndex = m_selectedIndex;
	if (CheckUseCapture()) 
	{
		m_usedCapture = true;
	}
	MainWindow::Get()->SetEnableInputAudioCapture(m_usedCapture);
	StartUpPopUp::show();
}

bool MicSettingWidget::CheckUseCapture()
{
	if (GetItemCount() != m_selectedIndex + 1) return true;
	else return false;
}

void MicSettingWidget::ItemRowChanged(int row)
{
	m_selectedIndex = row;

	bool selectNotUseMic = false;
	if (m_selectedIndex + 1 == ui.mic_listWidget->count())
	{
		selectNotUseMic = true;
	}

	ui.mute_checkbox->setDisabled(selectNotUseMic);
	ui.micVolume_slider->setDisabled(selectNotUseMic);

	MainWindow::Get()->GetStartupWindow()->SetMicSelection(m_selectedIndex);
}

QString MicSettingWidget::GetMicNameFromIndex(int index)
{
	QListWidgetItem* item = ui.mic_listWidget->item(index);
	MicSettingListItem* itemWidget = static_cast<MicSettingListItem*>(ui.mic_listWidget->itemWidget(item));
	return itemWidget->GetMicName();
}

void MicSettingWidget::SetSelectedIndex(int index)
{ 
	ui.mic_listWidget->setCurrentRow(index);
	m_selectedIndex = index; 
}


void MicSettingWidget::on_mute_checkbox_stateChanged(int state)
{
	MainWindow* pmainWindow = static_cast<MainWindow*>(App()->GetMainWindow());

	if (state == Qt::Checked) {
		// Set Muted
		ui.micVolume_slider->setValue(0);
	}
	else if (state == Qt::Unchecked) {
		// Set unMuted
		ui.micVolume_slider->setValue(m_lastMicVolume);
	}
}

bool MicSettingWidget::eventFilter(QObject* obj, QEvent* event)
{
	if (obj == ui.micVolume_slider) {
		if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick || event->type() == QEvent::MouseMove) {
			if (ui.micVolume_slider->isEnabled()) {
				QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
				int val = ui.micVolume_slider->minimum() + (ui.micVolume_slider->maximum() - ui.micVolume_slider->minimum()) * (static_cast<float>(mouseEvent->x()) / static_cast<float>(ui.micVolume_slider->width()));
				m_lastMicVolume = val;
				ui.micVolume_slider->setValue(val);
				event->ignore();
				return true;
			}
		}
		else if (event->type() == QEvent::Paint)
		{
			QPainter painter(ui.micVolume_slider);

			int _x_start = 0 + MICSET_SLIDER_ROUND_MARGIN;
			int _x_value = ui.micVolume_slider->width() * float(ui.micVolume_slider->value()) / 100.0;
			int _x_end = ui.micVolume_slider->width() - MICSET_SLIDER_ROUND_MARGIN;
			int _y = ui.micVolume_slider->height() / 2;

			if (ui.micVolume_slider->isEnabled())
			{
				if (ui.micVolume_slider->value() < ui.micVolume_slider->maximum()) {
					painter.setPen(QPen(GetAddpageColor(), 4, Qt::SolidLine, Qt::RoundCap));
					painter.drawLine(_x_value ? _x_value : MICSET_SLIDER_ROUND_MARGIN, _y, _x_end, _y);
				}

				if (ui.mute_checkbox->checkState() != Qt::Checked) {
					painter.setPen(QPen(GetSubpageColor(), 4, Qt::SolidLine, Qt::RoundCap));
					painter.drawLine(_x_start, _y, _x_value == ui.micVolume_slider->width() ? _x_value - MICSET_SLIDER_ROUND_MARGIN : _x_value, _y);
				}
			}
			else
			{
				painter.setPen(QPen(GetAddpageColor(), 4, Qt::SolidLine, Qt::RoundCap));
				painter.drawLine(MICSET_SLIDER_ROUND_MARGIN, _y, _x_end, _y);
			}

			if (m_sliderEnter && ui.micVolume_slider->isEnabled())
			{				
				_y = _y - MICSET_HANDLE_HEIGHT / 2;
				int _x = _x_value - MICSET_HANDLE_WIDTH / 2;

				if (_x < MICSET_HANDLE_WIDTH / 2) _x = 0;
				else if (_x > ui.micVolume_slider->width() - MICSET_HANDLE_WIDTH) _x = ui.micVolume_slider->width() - MICSET_HANDLE_WIDTH - 1;

				DrawHandleDownShadow(&painter, QRect(_x, _y, MICSET_HANDLE_WIDTH, MICSET_HANDLE_HEIGHT));

				painter.setPen(QPen(Qt::transparent, 1, Qt::NoPen, Qt::FlatCap));        // erase pen
				painter.setBrush(GetHandleColor());
				painter.drawRoundedRect(QRect(_x, _y, MICSET_HANDLE_WIDTH, MICSET_HANDLE_HEIGHT), 2.0, 2.0);
			}

			painter.end();
			
			event->ignore();
			return true;
		}
	}
	else if (obj == ui.slider_widget)
	{
		if (event->type() == QEvent::Enter)
		{
			SliderWidgetEnter(true);
		}
		else if (event->type() == QEvent::Leave)
		{
			SliderWidgetEnter(false);
		}
	}

	return StartUpPopUp::eventFilter(obj, event);
}

QT_BEGIN_NAMESPACE
extern Q_WIDGETS_EXPORT void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0);
QT_END_NAMESPACE

void MicSettingWidget::DrawHandleDownShadow(QPainter* painter, QRect& handle)
{
	handle.setX(handle.x() - HANDLE_SHADOW_DISTANCE / 2);
	handle.setY(handle.y() + HANDLE_SHADOW_DISTANCE / 2);

	QPixmap px(":/StartStreamDlg/image/slider_shadow_handle.png");
	QPoint offset(handle.x(), handle.y());

	// return if no source
	if (px.isNull())
		return;

	// save world transform
	QTransform restoreTransform = painter->worldTransform();
	painter->setWorldTransform(QTransform());

	// Calculate size for the background image
	QSize szi(px.size().width() + 2 * HANDLE_SHADOW_DISTANCE, px.size().height() + 2 * HANDLE_SHADOW_DISTANCE);

	QImage tmp(szi, QImage::Format_ARGB32_Premultiplied);
	QPixmap scaled = px.scaled(szi);
	tmp.fill(0);
	QPainter tmpPainter(&tmp);
	tmpPainter.setCompositionMode(QPainter::CompositionMode_Source);
	tmpPainter.drawPixmap(QPointF(-HANDLE_SHADOW_DISTANCE, -HANDLE_SHADOW_DISTANCE), scaled);
	tmpPainter.end();

	// blur the alpha channel
	QImage blurred(tmp.size(), QImage::Format_ARGB32_Premultiplied);
	blurred.fill(0);
	QPainter blurPainter(&blurred);
	qt_blurImage(&blurPainter, tmp, HANDLE_SHADOW_BLUR_RADIUS, false, true);
	blurPainter.end();

	tmp = blurred;

	// blacken the image...
	tmpPainter.begin(&tmp);
	tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
	tmpPainter.fillRect(tmp.rect(), HANDLE_SHADOW_COLOR);
	tmpPainter.end();

	// draw the blurred shadow...
	painter->drawImage(offset, tmp);

	// draw the actual pixmap...
	//painter->drawPixmap(offset, px, QRectF());

	// restore world transform
	painter->setWorldTransform(restoreTransform);
}

void MicSettingWidget::SliderWidgetEnter(bool enter)
{
	if (enter)
	{
		ui.slider_widget->setProperty("style", "1");
		m_sliderEnter = true;
	}
	else
	{
		ui.slider_widget->setProperty("style", "0");
		m_sliderEnter = false;
	}
	ui.slider_widget->style()->unpolish(ui.slider_widget);
	ui.slider_widget->style()->polish(ui.slider_widget);
}

void MicSettingWidget::HideEventCalled()
{
	SetCurrentIndex(m_lastSelectedIndex);
}

void MicSettingWidget::ShowEventCalled()
{

}

void MicSettingWidget::on_popup_close_btn_clicked()
{
	on_cancel_btn_clicked();
}

void MicSettingWidget::on_cancel_btn_clicked()
{
	// restore volume
	float MicVolume = 100.0;
	MicVolume = LocalSettings::GetMicVolume(MicVolume) * 100.0;
	ui.micVolume_slider->setValue(int(MicVolume));

	// restore mute
	bool mute = false;
	mute = LocalSettings::GetMicMute(mute);
	ui.mute_checkbox->setChecked(mute);

	// restore index
	SetCurrentIndex(m_lastSelectedIndex);
	StartUpPopUp::hide();
}

void MicSettingWidget::SetCurrentIndex(int index)
{
	MainWindow::Get()->GetStartupWindow()->SetMicSelection(index);
	ui.mic_listWidget->setCurrentRow(index);
}

void MicSettingWidget::on_select_btn_clicked()
{
	m_lastSelectedIndex = m_selectedIndex;
	SetCurrentIndex(m_selectedIndex);

	if (CheckUseCapture())
	{
		// Use Capture
		if (m_usedCapture)
		{
			// do nothing
		}
		else
		{
			m_usedCapture = true;
			MainWindow::Get()->SetEnableInputAudioCapture(m_usedCapture);
		}
	}
	else
	{
		// Stop Capture
		if (m_usedCapture)
		{
			m_usedCapture = false;
			MainWindow::Get()->SetEnableInputAudioCapture(m_usedCapture);
		}
		else
		{
			// do nothing
		}
	}

	// set Mic
	QString DeviceID = static_cast<MainWindow*>(App()->GetMainWindow())->GetStartupWindow()->GetMicDeviceID(m_selectedIndex);
	LocalSettings::SetSelectedMicDeviceID(DeviceID);

	// set Mic Muted
	Qt::CheckState state = ui.mute_checkbox->checkState();
	bool isMuted = false;
	if (state == Qt::Checked) {
		// Muted
		isMuted = true;
	}

	LocalSettings::SetMicMute(isMuted);

	// set Mic Volume
	if (m_lastMicVolume < 0) m_lastMicVolume = 0;
	else if (m_lastMicVolume > 100) m_lastMicVolume = 100;
	static_cast<MainWindow*>(App()->GetMainWindow())->SetMicVolume(float(m_lastMicVolume) / 100.0);

	StartUpPopUp::hide();
}

int MicSettingWidget::GetStartUpPopUpY()
{ 
	return (MainWindow::Get()->GetStartupWindow()->height() - MainWindow::Get()->GetStartupWindow()->getTitlebarHeight() - GetStartUpPopUpHeight()) / 2 + MainWindow::Get()->GetStartupWindow()->getTitlebarHeight();
}

int MicSettingWidget::GetStartUpPopUpHeight()
{ 
	int listWidgetHeight = ui.mic_listWidget->count() * MICSET_ITEM_HEIGHT + 20;
	int newh = listWidgetHeight + STARTPOPUP_CHILD_TOP_BOTTOM_MARGIN + MICSET_LABEL_MARGIN;

	if (newh > MainWindow::Get()->GetStartupWindow()->height() - MainWindow::Get()->GetStartupWindow()->getTitlebarHeight())
	{
		newh = MainWindow::Get()->GetStartupWindow()->height() - MainWindow::Get()->GetStartupWindow()->getTitlebarHeight() - STARTPOPUP_Y_MARGIN * 2;
	}

	return newh;
}

void MicSettingWidget::UpdateLanguageStringCalled()
{
	ui.selectMic_label->setText(AppString::GetQString("SlideUpDlg/select_mic_label"));
	ui.selectVolume_label->setText(AppString::GetQString("SlideUpDlg/select_mic_volume_label"));
	SetLabel(AppString::GetQString("SlideUpDlg/select_mic"));

	QListWidgetItem* item;
	MicSettingListItem* itemWidget;
	if (ui.mic_listWidget->count() > 1)
	{
		vector<AudioDeviceInfo> tempMicList;
		MainWindow::Get()->GetMicInfoList(tempMicList);
		auto& micItem = tempMicList.begin();

		item = ui.mic_listWidget->item(0);
		itemWidget = static_cast<MicSettingListItem*>(ui.mic_listWidget->itemWidget(item));
		itemWidget->SetMicName(AppString::GetQString("main_menu/main_menu_default_device_format").replace(QString("%s"), StringUtils::Local8bitToUnicode(micItem->name.c_str())));

		if (GetSelectedIndex() == 0)
		{
			MainWindow::Get()->GetStartupWindow()->SetMicSelection(0);
		}
	}

	item = ui.mic_listWidget->item(ui.mic_listWidget->count()-1);
	itemWidget = static_cast<MicSettingListItem*>(ui.mic_listWidget->itemWidget(item));
	itemWidget->SetMicName(AppString::GetQString("Common/un_used"));
}