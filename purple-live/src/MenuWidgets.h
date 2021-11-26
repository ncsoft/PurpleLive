#pragma once

#include "AppConfig.h"
#include "AppUtils.h"
#include "easylogging++.h"

#include <QWidget>
#include <QPainter>
#include <QStyleOption>

class MenuWidget: public QWidget
{
    Q_OBJECT
public:
	using QWidget::QWidget;

signals:
	void OnShow();
    void OnMouseHover();
	void OnMouseLeave();
	void OnMousePress();
	void OnMouseRelease();

protected:
	void showEvent(QShowEvent *event)
	{
		emit OnShow();
		QWidget::showEvent(event);
	}
    void enterEvent(QEvent *event)
	{
		emit OnMouseHover();
		QWidget::enterEvent(event);
	}
	void leaveEvent(QEvent *event)
	{
		emit OnMouseLeave();
		QWidget::leaveEvent(event);
	}
	void mousePressEvent(QMouseEvent *event)
	{
		emit OnMousePress();
		// 클릭 시 메뉴가 사라지지 않도록 처리
		//QWidget::mousePressEvent(event);
	}
	void mouseReleaseEvent(QMouseEvent *event)
	{
		emit OnMouseRelease();
		// 드래그 시 메뉴가 사라지지 않도록 처리
		//QWidget::mouseReleaseEvent(event);
	}
	void paintEvent(QPaintEvent *pe)
	{         
		// QWidget 서브 클래스의 qss 지원을 위해서 필요                                                                                                                             
		QStyleOption o;                                                                                                                                                                  
		o.initFrom(this);                                                                                                                                                                
		QPainter p(this);                                                                                                                                                                
		style()->drawPrimitive(QStyle::PE_Widget, &o, &p, this);                                                                                                                         
	};
};

#include <QPushButton>
class MenuButton: public QPushButton
{
    Q_OBJECT
public:
    using QPushButton::QPushButton;

signals:
    void OnMouseHover();
	void OnMouseLeave();

protected:
    void enterEvent(QEvent *event)
	{
		emit OnMouseHover();
		QPushButton::enterEvent(event);
	}
	void leaveEvent(QEvent *event)
	{
		emit OnMouseLeave();
		QPushButton::leaveEvent(event);
	}
};

class MenuButtonLicenseIcon : public MenuButton
{
	Q_OBJECT
	Q_PROPERTY(QColor iconColor WRITE SetIconColor READ GetIconColor)

public:
	using MenuButton::MenuButton;

private:
	QColor m_iconColor;
	QPixmap m_icon;

	void SetIconColor(QColor val)
	{
		m_icon = QPixmap(":/menu/image/menu/license.svg");
		m_iconColor = val;
		m_icon = DrawUtils::ChangeIconColor(m_icon, m_iconColor);
		setIcon(m_icon);
	}
	QColor GetIconColor() { return m_iconColor; }
};

#include <QCheckBox>
class MenuCheckBox: public QCheckBox
{
    Q_OBJECT
public:
    using QCheckBox::QCheckBox;

signals:
    void OnMouseHover();
	void OnMouseLeave();

protected:
    void enterEvent(QEvent *event)
	{
		emit OnMouseHover();
		QCheckBox::enterEvent(event);
	}
	void leaveEvent(QEvent *event)
	{
		emit OnMouseLeave();
		QCheckBox::leaveEvent(event);
	}
};

#include <QMenu>
class CustomMenu: public QMenu
{
    Q_OBJECT
	Q_PROPERTY(QColor separatorColor WRITE SetSeparatorColor READ GetSeparatorColor)
	Q_PROPERTY(QColor shadowColor WRITE SetShadowColor READ GetShadowColor)
public:
	using QMenu::QMenu;
	void SetSeparatorColor(QColor color) { m_separatorColor = color; }
	QColor GetSeparatorColor() { return m_separatorColor; }
	void SetShadowColor(QColor color) { m_shadowColor = color; }
	QColor GetShadowColor() { return m_shadowColor; }
	
signals:
    void OnMouseHover();
	void OnMouseLeave();
	
protected:
    void enterEvent(QEvent *event)
	{
		emit OnMouseHover();
		QMenu::enterEvent(event);
	}
	void leaveEvent(QEvent *event)
	{
		emit OnMouseLeave();
		QMenu::leaveEvent(event);
	}
	void paintEvent(QPaintEvent *e)
	{
		QMenu::paintEvent(e);
		QPainter painter(this);
		//elogd("[CustomMenu::paintEvent] count(%d)", this->children().count());
		for (int i=0; i<this->children().count(); i++)
		{
			QObject* current = this->children()[i];
			//elogd("[CustomMenu::paintEvent] index(%d) isWidgetType(%d) name(%s)", i, current->isWidgetType(), current->objectName().toStdString().c_str());
			if (current->isWidgetType() && current->objectName().toStdString()=="separator_menu_item_widget")
			{
				const QColor color = m_separatorColor;
				constexpr int margin = 11;
				QWidget* widget = qobject_cast<QWidget*>(current);
				int x = margin;
				int y = widget->pos().y() + (widget->size().height()/2);
				int w = width() - (margin*2);
				int h = 1;
				painter.fillRect(x, y, w, h, color);
			}	
		}
	}
private:
	QColor m_separatorColor;
	QColor m_shadowColor;
};

#include <QTimer>
class ChildMenu: public CustomMenu
{
    Q_OBJECT
public:
    ChildMenu(QWidget *parent = Q_NULLPTR) : CustomMenu{parent} 
	{ 
		initialize(); 
	}
    ChildMenu(const QString &title, QWidget *parent = Q_NULLPTR) : CustomMenu{title, parent} 
	{ 
		initialize(); 
	}

private:
	void initialize()
	{
		 connect(this, &ChildMenu::aboutToShow, this, &ChildMenu::onShowMenu);
	}

	bool isLeftPresent(QWidget* parent)
	{
		if (parent==nullptr)
			return false;

		if (parent->pos().x() > pos().x())
			return true;
		
		return false;
	}

private slots:
    void onShowMenu()
	{
		QTimer::singleShot(0, [this] { 
			// 부모 메뉴 좌측에 표시될 경우 3px 여백 제거 (QMenu의 기본 출력 위치 보정)
			if (isLeftPresent(parentWidget()))
			{
				move(pos() + QPoint(3, 0));
			}
		});
    }
};

#include <QSlider>
#include <QMouseEvent>
#include <QPainter>
class VolumeSlider : public QSlider
{
	Q_OBJECT
	Q_PROPERTY(QColor backgroundColor WRITE SetBackgroundColor READ GetBackgroundColor)
	Q_PROPERTY(QColor volumeColor WRITE SetVolumeColor READ GetVolumeColor)
	Q_PROPERTY(QColor magnitudeColor WRITE SetMagnitudeColor READ GetMagnitudeColor)
public:
	VolumeSlider(QWidget *parent = Q_NULLPTR) : QSlider{parent} {
		connect(this, &QSlider::valueChanged, this, [this](int value) {
			//elogd("[VolumeSlider::valueChanged value(%d) m_hover(%d) isEnabled(%d)", value, m_hover, isEnabled());
			if (!m_hover || !isEnabled())
				return;
			setShowThumb(true);
		});
	}

	void SetBackgroundColor(QColor color) { m_backgroundColor = color; }
	QColor GetBackgroundColor() { return m_backgroundColor; }
	void SetVolumeColor(QColor color) { m_volumeColor = color; }
	QColor GetVolumeColor() { return m_volumeColor; }
	void SetMagnitudeColor(QColor color) { m_magnitudeColor = color; }
	QColor GetMagnitudeColor() { return m_magnitudeColor; }

	void setSaveVolume(float value) { m_save_volume = value; }
	float getSaveVolume() { return m_save_volume; }

	void setMagnitude(float value) {
		if (m_magnitude==value)
			return;
		m_magnitude = value;
		repaint();
	}
	float getMagnitude() { return m_magnitude; }
	
protected:
	void setShowThumb(bool show)
	{
		if (show)
			setProperty("show_thumb", true);
		else
			setProperty("show_thumb", false);
		style()->unpolish(this);
		style()->polish(this);
	}
	void setHover(bool hover)
	{
		m_hover = hover;
		bool show = m_hover && isEnabled();
		setShowThumb(show);
	}
	void enterEvent(QEvent *event)
	{
		//elogd("[MenuTooltip::enterEvent]");
		setHover(true);
	}
	void leaveEvent(QEvent *event)
	{
		//elogd("[MenuTooltip::leaveEvent]");
		setHover(false);
	}
	void mousePressEvent(QMouseEvent *event)
	{
		QStyleOptionSlider opt;
		initStyleOption(&opt);
		QRect sr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
		if (event->button() == Qt::LeftButton && sr.contains(event->pos()) == false)
		{
			int newVal;
			if (orientation() == Qt::Vertical)
				newVal = minimum() + ((maximum()-minimum()) * (height()-event->y())) / height();
			else
				newVal = minimum() + ((maximum()-minimum()) * event->x()) / width();

			if (invertedAppearance() == true)
				setValue( maximum() - newVal );
			else
				setValue(newVal);

			event->accept();
		}
		QSlider::mousePressEvent(event);
	}

	void drawBackground(QPainter& painter, QColor& color, float value)
	{
		constexpr int start_position_y = 14;
		constexpr int minimum_w = 4;
		constexpr int thumb_w = 8;
		constexpr int radius = 2;
		int x = 0;
		int y = start_position_y;
		int w = value * (width()-thumb_w) + ceil(thumb_w*value);
		w = max(w, minimum_w);
		int h = 4;

		if (value>0)
		{
			painter.setPen(color);
			painter.setBrush(color);
			painter.drawEllipse(QPointF(x+2,y+2), 1.5, 1.5);
			painter.drawRect(x+radius, y, w-(radius*2), h-1);
			painter.drawEllipse(QPointF(w-2,y+2), 1.5, 1.5);
		}
	}
	void paintEvent(QPaintEvent *e)
	{
		QPainter painter(this);

		float magnitude = m_magnitude;
		float volume = (float)value()/100;

		QColor m_background_color = QColor(GetBackgroundColor());
		QColor m_volume_color = QColor(GetVolumeColor());
		QColor m_magnitude_color = QColor(GetMagnitudeColor());
		
		if (!isEnabled()) {
			m_background_color = QColor(GetBackgroundColor());
			m_volume_color = QColor(GetBackgroundColor());
			m_magnitude_color = QColor(GetBackgroundColor());
		}
		
		drawBackground(painter, m_background_color, 1.0f);
		drawBackground(painter, m_volume_color, volume);
		drawBackground(painter, m_magnitude_color, volume*magnitude);

		painter.end();
		QSlider::paintEvent(e);
	}

private:
	bool m_hover = false;
	QColor m_backgroundColor;
	QColor m_volumeColor;
	QColor m_magnitudeColor;
	float m_save_volume = 0.0f;
	float m_magnitude = 0.0f;
};