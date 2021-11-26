#pragma once
#include "MainWindow.h"
#include "StartUpWindow.h"

#include <qpushbutton.h>

class ScreenLockWidget : public QWidget {
	Q_OBJECT
	Q_PROPERTY(int iconToButtonMargin WRITE SetIconToButtonMargin READ GetIconToButtonMargin)
	Q_PROPERTY(QColor backgroundColor WRITE SetBackgroundColor READ GetBackgroundColor)
	Q_PROPERTY(QPixmap pixmapIcon WRITE SetPixmapIcon READ GetPixmapIcon)
	Q_PROPERTY(QSize buttonSize WRITE SetButtonSize READ GetButtonSize)
	Q_PROPERTY(QSize iconSize WRITE SetIconSize READ GetIconSize)

public:
	explicit ScreenLockWidget(QWidget *parent = 0);
	virtual ~ScreenLockWidget() {}

	void Show();
	void Hide();
	bool IsActive();

	static QPointer<ScreenLockWidget> Instance();

signals:
	void SendScreenLockShowEvent();
	void SendScreenLockHidedEvent();

private:
	// Child Object
	QPushButton* m_screenlockBtn;
	int m_iconToButtonHeight{0};

	void ActiveWidget(bool active);

	// q-property
	int m_iconToButtonMargin;
	QColor m_backgroundColor;
	QPixmap	m_pixmapIcon;
	QSize m_buttonSize;
	QSize m_iconSize;

	void SetIconToButtonMargin(int val) { m_iconToButtonMargin = val; }
	int  GetIconToButtonMargin() { return m_iconToButtonMargin; }
	void SetBackgroundColor(QColor val) { m_backgroundColor = val; }
	QColor GetBackgroundColor() { return m_backgroundColor; }
	void SetPixmapIcon(QPixmap val) { m_pixmapIcon = val; }
	QPixmap GetPixmapIcon() { return m_pixmapIcon; }
	void SetButtonSize(QSize val);
	QSize GetButtonSize() { return m_buttonSize; }
	void SetIconSize(QSize val) { m_iconSize = val; }
	QSize GetIconSize() { return m_iconSize; }

private slots:
	bool eventFilter(QObject* obj, QEvent* event);
	void closeEvent(QCloseEvent* e);
	void paintEvent(QPaintEvent * e);
	void showEvent(QShowEvent* e);
	void ResetGeometry();
	void UpdateLanguageString();
};