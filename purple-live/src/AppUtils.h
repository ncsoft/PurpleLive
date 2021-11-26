#pragma once
#include <string>
#include <qstring.h>
#include <qfont.h>
#include <qcolor.h>
#include <qscreen.h>
#include <qwidget.h>
#include <Windows.h>
#include <functional>

struct AppUtils {
	static void InitLog();
	static std::string GetAppDataPath();
	static std::string GetLogPath();
	static std::string GetWebviewCachePath();
	static std::string GetWebviewPersistPath();
	static std::string format(const char *format, ...);
	static void DispatchToMainThread(std::function<void()> callback);
	static bool fileExists(QString path);
	static QScreen* GetActiveScreen(QWidget* widget);
	static QRect GetScreenCenterGeometry(QWidget* widget, int width, int height);
	static bool VerifyWindowRect(QWidget* widget, QRect& rect);
	static RECT GetAspectRatioRect(int sx, int sy, int dx, int dy);
	static QFont GetRegularFont(int fontSize = 10);
	static QFont GetMediumFont(int fontSize = 10);
	static QFont GetBoldFont(int fontSize = 10);
};

struct StringUtils {
	static bool IsEqualNoCase(std::string s1, std::string s2);
	static bool StartWith(std::string s, std::string match);
	static std::string GetStringFromUtf8(std::string s, const char* codecForName = "eucKR");
	static std::string GetStringFromUtf8(std::wstring s, const char* codecForName = "eucKR");
	static std::string ValueOrEmpty(const char* s);
	static std::wstring StringToWString(std::string);
	static std::string WStringToString(std::wstring);

	static std::string wstring_to_string(std::wstring);
	static std::wstring string_to_wstring(std::string);
	static std::string utf8_to_string(std::string);
	static std::wstring utf8_to_wstring(std::string);

	static std::string& trim(std::string& str, const std::string& chars = "\t\n\v\f\r ");

	static QString Local8bitToUnicode(const char* local8bit, const char* codecForName = "eucKR");
	static QString Local8bitToUnicode(QByteArray& local8bit, const char* codecForName = "eucKR");
};

struct DumpUtils
{
	static void initialize_crach_handler();
	static LONG CALLBACK exception_handler(PEXCEPTION_POINTERS exception);
};

struct DrawUtils
{
	static void			DrawRectImage(QPixmap* pixmap, QPainter* painter, QRect& rect);
	static void			DrawEllipseImage(QPixmap* pixmap, QPainter* painter, QRect& rect, QColor borderPen = Qt::NoPen);
	static void			DrawSquicleImage(QPixmap* srcPixmap, QPixmap* desPixmap, QPainter* painter, QRect& rect);
	static void			DrawRotatedImage(QPixmap* pixmap, QPainter* painter, QRect& rect, int rotateAngle = 0);
	static int			GetFontWidth(QFont& font, QString& printText);
	static int			GetFontHeight(QFont& font);
	static QString		ChangeDottedStr(QString& ori, int maxWidth, QFont& font);
	static QPixmap		ChangeIconColor(QString& _path, QColor& _color = QColor(255, 255, 255));
	static QPixmap		ChangeIconColor(QPixmap _pixmap, QColor& _color = QColor(255, 255, 255));
};