#pragma once

#include <qslider.h>

#define MIC_SLIDER_HEIGHT_MARGIN			3					// px
#define MIC_SLIDER_WIDTH_MARGIN				5					// px
#define MIC_SLIDER_VOLUME_HEIGHT			5					// px
#define MIC_SLIDER_WIDTH					8					// px
#define MIC_SLIDER_HEIGHT					15					// px

class MicSlider : public QSlider {
	Q_OBJECT
	Q_PROPERTY(QColor micSliderHandle WRITE SetMicSliderHandle READ GetMicSliderHandle)
	Q_PROPERTY(QColor micSliderHandleDiable WRITE SetMicSliderHandleDiable READ GetMicSliderHandleDiable)
	Q_PROPERTY(QColor micSliderVolumeLevel WRITE SetMicSliderVolumeLevel READ GetMicSliderVolumeLevel)

public:
	explicit MicSlider(QWidget *parent = Q_NULLPTR);
	~MicSlider() {}

	void StopPaintInputVolume(bool b) { m_stopPaintInputMic = b; }

public slots:
	void SetMicLevel(unsigned int level);
	int GetMicLevel() { return m_micLevel; }

protected:
	virtual void paintEvent(QPaintEvent *e) override;

private:
	int m_micLevel = 0;
	bool m_stopPaintInputMic = false;
	
	// q-property value, func
	QColor m_micSliderHandle;
	QColor m_micSliderHandleDiable;
	QColor m_micSliderVolumeLevel;

	void SetMicSliderHandle(QColor color) { m_micSliderHandle = color; }
	void SetMicSliderHandleDiable(QColor color) { m_micSliderHandleDiable = color; }
	void SetMicSliderVolumeLevel(QColor color) { m_micSliderVolumeLevel = color; }

	QColor GetMicSliderHandle() { return m_micSliderHandle; }
	QColor GetMicSliderHandleDiable() { return m_micSliderHandleDiable; }
	QColor GetMicSliderVolumeLevel() { return m_micSliderVolumeLevel; }
};
