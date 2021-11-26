#include "MicSlider.h"

#include <qpainter.h>

MicSlider::MicSlider(QWidget *parent)
	: QSlider(parent)
{
}

void MicSlider::SetMicLevel(unsigned int level)
{
	// Set this value : 0 ~ 99

	if (m_stopPaintInputMic) {
		m_micLevel = 0;
	}
	else {
		m_micLevel = level;

		if (m_micLevel >= 100) {
			m_micLevel = 100;
		}

		m_micLevel = width() * m_micLevel / 100;
	}
}

void MicSlider::paintEvent(QPaintEvent *e)
{
	QSlider::paintEvent(e);

	QPainter painter(this);

	painter.setBrush(m_micSliderVolumeLevel);
	painter.setPen(m_micSliderVolumeLevel);

	if(m_micLevel > 0) painter.drawRect(0, height() / 2 - MIC_SLIDER_VOLUME_HEIGHT - MIC_SLIDER_HEIGHT_MARGIN, m_micLevel - MIC_SLIDER_WIDTH_MARGIN, MIC_SLIDER_VOLUME_HEIGHT);

	if (isEnabled()) {
		painter.setBrush(m_micSliderHandle);
		painter.setPen(m_micSliderHandle);
	}
	else {
		painter.setBrush(m_micSliderHandleDiable);
		painter.setPen(m_micSliderHandleDiable);
	}
	painter.drawRect(width() * value() / 100 - MIC_SLIDER_WIDTH + ((1.0 - float(value()) / 100.0) * MIC_SLIDER_WIDTH), height() / 2 - (MIC_SLIDER_HEIGHT / 2), MIC_SLIDER_WIDTH, MIC_SLIDER_HEIGHT);

	painter.end();
}
