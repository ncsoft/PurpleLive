#include "StartUpLauncherWindow.h"
#include <Windows.h>

#include <windows.h>		// for ::PrintWindow
#include <QtWin>			// for QtWin::
#include <qpainter.h>		// for QPainter
#include <QPrinter>
#include <qicon.h>

#include "AppString.h"
#include "VideoChatApp.h"
#include "LocalSettings.h"
#include "AppUtils.h"

StartUpLauncherWindow::StartUpLauncherWindow(QWidget *parent)
	: QFramelessDialog(parent)
{
	ui.setupUi(this);

	this->setWindowIcon(QIcon(QString::fromStdString(AppConfig::Instance()->purplelive_icon_path)));
	this->setWindowTitle(QString::fromStdString(AppString::GetQString("Common/service_name").toStdString()));

	QListWidget* listWidget = ui.tabWidget->findChild<QListWidget*>("listWidget_window");

	if (listWidget)
	{
		listWidget->setViewMode(QListWidget::IconMode);
		listWidget->setIconSize(QSize(188, 148));
		listWidget->setGridSize(QSize(200, 200));
//		listWidget->setResizeMode(QListWidget::Adjust);
	}
}

StartUpLauncherWindow::~StartUpLauncherWindow()
{
}

bool StartUpLauncherWindow::GetActive()
{
	return m_active;
}

void StartUpLauncherWindow::SetActive(bool enable)
{
	m_active = enable;
	if (enable)
	{
		show();
	}
	else
	{
		deleteLater();
	}
}

bool StartUpLauncherWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
#if (QT_VERSION == QT_VERSION_CHECK(5, 11, 1))
	MSG* msg = *reinterpret_cast<MSG**>(message);
#else
	MSG* msg = reinterpret_cast<MSG*>(message);
#endif

	switch (msg->message)
	{
	case WM_MOVE:
	case WM_SIZING:
		m_changed_size_or_pos = true;
		break;
	}
	return QFramelessDialog::nativeEvent(eventType, message, result);
}

bool StartUpLauncherWindow::event(QEvent *event)
{
	switch (event->type())
	{
	case QEvent::MouseButtonRelease:
		if (m_changed_size_or_pos)
		{
			m_changed_size_or_pos = false;
			LocalSettings::SetWindowGeometry(metaObject()->className(), geometry());
		}
		break;
	}

	return QFramelessDialog::event(event);
}

BOOL CALLBACK WindowsEnumerationHandler(HWND hwnd, LPARAM param)
{
	// Skip windows that are invisible, minimized, have no title, or are owned,
	// unless they have the app window style set.
	int len = GetWindowTextLength(hwnd);
	HWND owner = GetWindow(hwnd, GW_OWNER);
	LONG exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);
	if (len == 0 || IsIconic(hwnd) || !IsWindowVisible(hwnd) ||
		(owner && !(exstyle & WS_EX_APPWINDOW))) {
		return TRUE;
	}

	// Skip the Program Manager window and the Start button.
	const size_t kClassLength = 256;
	WCHAR class_name[kClassLength];
	const int class_name_length = GetClassName(hwnd, class_name, kClassLength);

	// Skip Program Manager window and the Start button. This is the same logic
	// that's used in Win32WindowPicker in libjingle. Consider filtering other
	// windows as well (e.g. toolbars).
	if (wcscmp(class_name, L"Progman") == 0 || wcscmp(class_name, L"Button") == 0)
		return TRUE;

	// TODO: 현재 사용 중인 Virtual Desktop의 Window가 아닌 경우 제외
	WCHAR window_title[MAX_PATH];
	// Truncate the title if it's longer than kTitleLength.
	GetWindowText(hwnd, window_title, MAX_PATH);

	StartUpLauncherWindow* window = reinterpret_cast<StartUpLauncherWindow*>(param);
	window->AddThumbnail(window_title, hwnd);

	DWORD lpdwProcessId;
	GetWindowThreadProcessId(hwnd, &lpdwProcessId);

	return TRUE;
}

void StartUpLauncherWindow::UpdateGameList()
{
	LPARAM param = reinterpret_cast<LPARAM>(this);
	if (!EnumWindows(&WindowsEnumerationHandler, param))
		return;

	QRect default_rect = AppUtils::GetScreenCenterGeometry(this, width(), height());	
	QRect saved_rect = LocalSettings::GetWindowGeometry(metaObject()->className(), default_rect);
	if (!AppUtils::VerifyWindowRect(this, saved_rect))
	{
		saved_rect = default_rect;
	}
	move(saved_rect.left(), saved_rect.top());
}

void StartUpLauncherWindow::AddThumbnail(const std::wstring appName, HWND hwnd)
{
	QListWidget* listWidget = ui.tabWidget->findChild<QListWidget*>("listWidget_window");

	if (listWidget)
	{
		QListWidgetItem* item = new QListWidgetItem(QString::fromWCharArray(appName.c_str()));
		item->setData(Qt::UserRole, QVariant::fromValue((WId)hwnd));
		listWidget->addItem(item);
		QPixmap p = GetCapturePixmap(hwnd);
		item->setIcon(QIcon(p));
	}
}

QPixmap StartUpLauncherWindow::GetCapturePixmap(HWND hwnd)
{
	HWND currentHwnd = hwnd;

	RECT rect;
	::GetWindowRect(currentHwnd, &rect);
	int captured_width = rect.right - rect.left;
	int captured_height = rect.bottom - rect.top;

	HDC happDC = ::GetWindowDC(currentHwnd);
	HDC hMemDC = ::CreateCompatibleDC(happDC);

	// dpi강제 조정하면 width, height가 실제 그려지는 값보다 크게 들어옴.
	int get_width = captured_width - 1;
	while (get_width > 0) {
		COLORREF ref;
		ref = ::GetPixel(happDC, get_width--, captured_height / 2);

		unsigned char red = ref & 0x80;
		unsigned char green = (ref >> 8) & 0x80;
		unsigned char blue = (ref >> 16) & 0x80;
		unsigned char alpha = (ref >> 24) & 0x80;

		if (red == 128 && green == 128 && blue == 128 && alpha == 128) {
		}
		else {
			captured_width = get_width + 1;
			break;
		}
	}

	int get_height = captured_height - 10;
	while (get_height > 0) {
		COLORREF ref;
		ref = ::GetPixel(happDC, captured_width / 2, get_height--);

		unsigned char red = ref & 0x80;
		unsigned char green = (ref >> 8) & 0x80;
		unsigned char blue = (ref >> 16) & 0x80;
		unsigned char alpha = (ref >> 24) & 0x80;

		if (red == 128 && green == 128 && blue == 128 && alpha == 128) {
		}
		else {
			captured_height = get_height + 1;
			break;
		}
	}

	HBITMAP hBitmap = CreateCompatibleBitmap(happDC, captured_width, captured_height);
	HBITMAP hMemBitmap = static_cast<HBITMAP>(::SelectObject(hMemDC, hBitmap));

	BOOL res = ::PrintWindow(currentHwnd, hMemDC, 2);


	QPixmap pixmap = QtWin::fromHBITMAP(hBitmap);

	// 메모리 정리
	::SelectObject(hMemDC, hMemBitmap);
	::DeleteObject(hBitmap);
	::DeleteDC(hMemDC);
	::ReleaseDC(currentHwnd, happDC);

	return pixmap;
}

void StartUpLauncherWindow::on_pushButton_start_clicked()
{
	// for Capture Test QA
	QListWidget* listWidget = ui.tabWidget->findChild<QListWidget*>("listWidget_window");

	if (listWidget)
	{
		listWidget->clear();
		UpdateGameList();
	}
}

void StartUpLauncherWindow::on_listWidget_window_itemDoubleClicked(QListWidgetItem *item)
{
	QVariant data = item->data(Qt::UserRole);
	HWND hwnd = (HWND)(LONG_PTR)data.toInt();

	MainWindow* mainWindow = MainWindow::Get();

	if (mainWindow)
	{
		App()->SetGameHwnd(hwnd);
		mainWindow->SetActiveStartUpLauncherWindow(false);

		if (App()->GetForceExcute())
		{
			if (mainWindow->StartRender(App()->GetForceExcute(), App()->GetRoomID().c_str()))
			{
				SetActive(false);
				mainWindow->SetActive(true);
			}
		}
		else
		{
			SetActive(false);
			mainWindow->SetActiveStartUpGameWindow(true);
		}
	}
}
