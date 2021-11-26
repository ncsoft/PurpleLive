#include "AppUtils.h"
#include "AppConfig.h"
#include <string>
#include <iostream>
#include <stdarg.h>
#include "easylogging++.h"
#include <atlcore.h>

#include "qfileinfo.h"		// for check qss file
#include <qwindow.h>
#include <qdesktopwidget.h>
#include <qapplication.h>
#include <qtimer.h>
#include <qpainter.h>
#include <qsettings.h>
#include <QTextCodec>
#include "shlobj.h"

void AppUtils::InitLog()
{
	elpp_set_path(GetLogPath().c_str());
	elpp_set_enable_log(AppConfig::enable_log_error, AppConfig::enable_log_info, AppConfig::enable_log_debug);
}

std::string AppUtils::GetAppDataPath()
{
	char szPath[MAX_PATH];
	SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, szPath);
	return szPath;
}

std::string AppUtils::GetLogPath()
{
	return GetAppDataPath() + "\\" + AppConfig::log_save_path;
}

std::string AppUtils::GetWebviewCachePath()
{
	return GetAppDataPath() + "\\" + AppConfig::webview_cache_path;
}

std::string AppUtils::GetWebviewPersistPath()
{
	return GetAppDataPath() + "\\" + AppConfig::webview_persist_path;
}

std::string AppUtils::format(const char *format, ...)
{
	va_list args;
	va_start(args, format);
#ifndef _MSC_VER
	size_t size = std::snprintf(nullptr, 0, format, args) + 1; // Extra space for '\0'
	std::unique_ptr<char[]> buf(new char[size]);
	std::vsnprintf(buf.get(), size, format, args);
	return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
#else
	int size = _vscprintf(format, args);
	std::string result(++size, 0);
	vsnprintf_s((char *)result.data(), size, _TRUNCATE, format, args);
	return result;
#endif
	va_end(args);
}

bool AppUtils::fileExists(QString path)
{
	QFileInfo check_file(path);
	if (check_file.exists() && check_file.isFile()) {
		return true;
	}
	else {
		return false;
	}
}

void AppUtils::DispatchToMainThread(std::function<void()> callback)
{
    // any thread
    QTimer* timer = new QTimer();
    timer->moveToThread(qApp->thread());
    timer->setSingleShot(true);
    QObject::connect(timer, &QTimer::timeout, [=]()
    {
        // main thread
        callback();
        timer->deleteLater();
    });
    QMetaObject::invokeMethod(timer, "start", Qt::QueuedConnection, Q_ARG(int, 0));
}

QScreen* AppUtils::GetActiveScreen(QWidget* widget)
{
	QScreen* active_screen = nullptr;

	while (widget)
	{
		auto window = widget->windowHandle();
		if (window != nullptr)
		{
			active_screen = window->screen();
			break;
		}
		else
		{
			widget = widget->parentWidget();
		}
	}

	return active_screen;
}

QRect AppUtils::GetScreenCenterGeometry(QWidget* widget, int width, int height)
{
	QScreen *screen = GetActiveScreen(widget);
	QRect screen_rect = screen->geometry();
	return QRect(screen_rect.width() / 2 - width / 2, screen_rect.height() / 2 - height / 2, width, height);
}

bool AppUtils::VerifyWindowRect(QWidget* widget, QRect& rect)
{
	for (int i=0; i<QApplication::desktop()->screenCount(); i++)
	{
		QRect screen_rect = QApplication::desktop()->screenGeometry(i);
		if (screen_rect.contains(rect.left(), rect.top()))
			return true;
		if (screen_rect.contains(rect.right(), rect.top()))
			return true;
	}
	return false;
}

RECT AppUtils::GetAspectRatioRect(int sx, int sy, int dx, int dy)
{
	double ratio_src = (double)sx / sy;
	double ratio_dst = (double)dx / dy;
	
	int x, y, w, h;
	if (ratio_src > ratio_dst)
	{
		// letter-box : top, bottom
		w = dx;
		h = round((double)w / ratio_src);
		x = 0;
		y = (dy - h) / 2;
	}
	else
	{
		// letter-box : left, right
		h = dy;
		w = round(ratio_src * h);
		x = (dx - w) / 2;
		y = 0;
	}

	RECT rc{x, y, w+x, h+y};
	//LOG(DEBUG) << format_string("[AppUtils::GetAspectRatioRect] (%d %d) (%d %d)", x, y, w, h);
	return rc;
}

bool StringUtils::IsEqualNoCase(std::string s1, std::string s2)
{
   //convert s1 and s2 into lower case strings
   transform(s1.begin(), s1.end(), s1.begin(), ::tolower);
   transform(s2.begin(), s2.end(), s2.begin(), ::tolower);
   if(s1.compare(s2) == 0)
      return true; //The strings are same
   return false; //not matched
}

bool StringUtils::StartWith(std::string s, std::string match)
{
    if(s.find(match) == 0)
        return true;
    else
        return false;
}

std::string StringUtils::GetStringFromUtf8(std::string s, const char* codecForName)
{
	QTextCodec *codec = QTextCodec::codecForName(codecForName);
	return codec->fromUnicode(s.c_str()).toStdString();	
}

std::string StringUtils::GetStringFromUtf8(std::wstring ws, const char* codecForName)
{
	QTextCodec *codec = QTextCodec::codecForName(codecForName);
	return codec->fromUnicode(ws.c_str()).toStdString();
}

std::string StringUtils::ValueOrEmpty(const char* s)
{
    return s == nullptr ? std::string() : s;
}

std::wstring StringUtils::StringToWString(std::string s)
{
	std::wstring res;
	res.assign(s.begin(), s.end());
	return res;
}

std::string StringUtils::WStringToString(std::wstring s)
{
	std::string res;
	res.assign(s.begin(), s.end());
	return res;
}

std::string StringUtils::wstring_to_string(std::wstring s)
{
	USES_CONVERSION;
	return std::string(W2A(s.c_str()));
}

std::wstring StringUtils::string_to_wstring(std::string s)
{
	USES_CONVERSION;
	return std::wstring(A2W(s.c_str()));
}

std::string StringUtils::utf8_to_string(std::string s)
{
	USES_CONVERSION;
	return wstring_to_string(utf8_to_wstring(s));
}

std::wstring StringUtils::utf8_to_wstring(std::string s)
{
	USES_CONVERSION;
	return std::wstring(A2W_CP(s.c_str(), CP_UTF8));
}

std::string& ltrim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
	str.erase(0, str.find_first_not_of(chars));
	return str;
}

std::string& rtrim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
	str.erase(str.find_last_not_of(chars) + 1);
	return str;
}

std::string& StringUtils::trim(std::string& str, const std::string& chars/*= "\t\n\v\f\r "*/)
{
	return ltrim(rtrim(str, chars), chars);
}

QString StringUtils::Local8bitToUnicode(const char* local8bit, const char* codecForName)
{
	QByteArray array = local8bit;
	return Local8bitToUnicode(array, codecForName);
}

QString StringUtils::Local8bitToUnicode(QByteArray& local8bit, const char* codecForName)
{
	return QTextCodec::codecForName(codecForName)->toUnicode(local8bit);
}

#include <windows.h>
#include <DbgHelp.h>
#include <atlstr.h>

#pragma comment (lib, "dbghelp.lib")

std::string& GetTimeDateFilename(const char *path, const char*name, const char *extension, bool noSpace)
{
	time_t now = time(0);
	char file[256] = {};
	struct tm *cur_time;

	cur_time = localtime(&now);
	snprintf(file, sizeof(file), "%s\\%s_%d-%02d-%02d%c%02d-%02d-%02d.%s",
		path, name,
		cur_time->tm_year + 1900, cur_time->tm_mon + 1,
		cur_time->tm_mday, noSpace ? '_' : ' ', cur_time->tm_hour,
		cur_time->tm_min, cur_time->tm_sec, extension);

	static std::string filename;
	filename = file;
	return filename;
}

void DumpUtils::initialize_crach_handler()
{
	static bool initialized = false;

	if (!initialized)
	{
		SetUnhandledExceptionFilter(exception_handler);
		initialized = true;
	}
}

LONG DumpUtils::exception_handler(PEXCEPTION_POINTERS exception)
{
	const std::string dump_dir_name = "\\logs\\";

	TCHAR path[MAX_PATH]; 
	::GetModuleFileName(0, path, _MAX_PATH); 
	TCHAR* p = _tcsrchr(path, '\\'); 
	path[p - path] = 0;

	std::wstring _dir = path;
	std::string dir(_dir.begin(), _dir.end());
	dir += dump_dir_name;
	std::string filename = GetTimeDateFilename(dir.c_str(), "dump", "dmp", true);
	std::wstring _filename(filename.begin(), filename.end());

	HANDLE hFile = CreateFile(_filename.c_str(), GENERIC_ALL, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (!hFile)
	{
		LOG(ERROR) << "Failed to write dump: Invalid dump file";
	}
	else
	{
		_MINIDUMP_EXCEPTION_INFORMATION exInfo;

		exInfo.ThreadId = ::GetCurrentThreadId();
		exInfo.ExceptionPointers = exception;
		exInfo.ClientPointers = NULL;

		const DWORD Flags = MiniDumpWithFullMemory | MiniDumpWithFullMemoryInfo | MiniDumpWithHandleData | MiniDumpWithUnloadedModules | MiniDumpWithThreadInfo;

		HANDLE hProcess = GetCurrentProcess();

		BOOL Result = MiniDumpWriteDump(hProcess, GetProcessId(hProcess), hFile, (MINIDUMP_TYPE)Flags, &exInfo, nullptr, nullptr);

		CloseHandle(hFile);

		if (!Result)
		{
			DWORD errorNo = GetLastError();
			LOG(ERROR) << format_string("MiniDumpWriteDump failed (%d)", errorNo);
		}
	}

	return 0;
}

void DrawUtils::DrawRectImage(QPixmap* pixmap, QPainter* painter, QRect& rect)
{
	QPixmap scaled = pixmap->scaled(rect.width(), rect.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

	QImage fixedImage(rect.width(), rect.height(), QImage::Format_ARGB32_Premultiplied);
	fixedImage.fill(Qt::transparent);
	QPainter imgPainter(&fixedImage);
	QPainterPath clip;
	clip.addRect(0, 0, rect.width(), rect.height());
	imgPainter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, true);
	imgPainter.setClipPath(clip);
	imgPainter.drawPixmap(0, 0, rect.width(), rect.height(), scaled);
	imgPainter.end();

	painter->drawPixmap(rect.x(),
		rect.y(),
		rect.width(),
		rect.height(),
		QPixmap::fromImage(fixedImage));
}

void DrawUtils::DrawSquicleImage(QPixmap* srcPixmap, QPixmap* desPixmap, QPainter* painter, QRect& rect)
{
	// scaled input image
	QPixmap srcScaled = srcPixmap->scaled(rect.width(), rect.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
	QPixmap desScaled = desPixmap->scaled(rect.width(), rect.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

	// crop image
	QImage fixedImage(rect.width(), rect.height(), QImage::Format_ARGB32_Premultiplied);
	fixedImage.fill(Qt::transparent);
	QPainter imgPainter(&fixedImage);

	imgPainter.setCompositionMode(QPainter::CompositionMode_Source);
	imgPainter.fillRect(rect, Qt::transparent);

	imgPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
	imgPainter.drawImage(0, 0, QImage(desScaled.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied)));

	imgPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
	imgPainter.drawImage(0, 0, QImage(srcScaled.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied)));

	// draw image
	painter->drawPixmap(rect.x(),
		rect.y(),
		rect.width(),
		rect.height(),
		QPixmap::fromImage(fixedImage));
}

void DrawUtils::DrawRotatedImage(QPixmap* pixmap, QPainter* painter, QRect& rect, int rotateAngle)
{
	// scale Image
	QPixmap scaled = pixmap->scaled(rect.width(), rect.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

	// rotate Image
	QPixmap rotatedPixmap(QSize(rect.width(), rect.height()));
	rotatedPixmap.fill(Qt::transparent);
	std::unique_ptr<QPainter> p = std::make_unique<QPainter>(&rotatedPixmap);
	p->translate(rect.height() / 2, rect.height() / 2);
	p->rotate(rotateAngle);
	p->translate(-rect.height() / 2, -rect.height() / 2);
	p->drawPixmap(0, 0, scaled);
	p->end();

	// draw image
	QImage fixedImage(rect.width(), rect.height(), QImage::Format_ARGB32_Premultiplied);
	fixedImage.fill(0);
	QPainter imgPainter(&fixedImage);
	QPainterPath clip;
	clip.addRect(0, 0, rect.width(), rect.height());
	imgPainter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, true);
	imgPainter.setClipPath(clip);
	imgPainter.drawPixmap(0, 0, rect.width(), rect.height(), rotatedPixmap);
	imgPainter.end();

	painter->drawPixmap(
		rect.x(),
		rect.y(),
		rect.width(),
		rect.height(),
		QPixmap::fromImage(fixedImage));
}

void DrawUtils::DrawEllipseImage(QPixmap* pixmap, QPainter* painter, QRect& rect, QColor borderPen)
{
	if (borderPen != Qt::NoPen)
	{
		painter->setPen(borderPen);
		painter->drawEllipse(rect.x(), rect.y(), rect.width(), rect.height());
	}
 
	QPixmap scaled = pixmap->scaled(rect.width(), rect.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

	QImage fixedImage(rect.width(), rect.height(), QImage::Format_ARGB32_Premultiplied);
	fixedImage.fill(Qt::transparent);
	QPainter imgPainter(&fixedImage);
	QPainterPath clip;
	clip.addRoundedRect(0, 0, rect.width(), rect.height(), rect.width() / 2, rect.height() / 2);
	imgPainter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, true);
	imgPainter.setClipPath(clip);
	imgPainter.drawPixmap(0, 0, rect.width(), rect.height(), scaled);
	imgPainter.end();

	painter->drawPixmap(rect.x(),
		rect.y(),
		rect.width(),
		rect.height(),
		QPixmap::fromImage(fixedImage));
}

int DrawUtils::GetFontWidth(QFont& font, QString& printText)
{
	QFontMetrics fm(font);
	int newW = fm.horizontalAdvance(printText);

	return newW;
}

int DrawUtils::GetFontHeight(QFont& font)
{
	QFontMetrics fm(font);
	return fm.height();
}

QPixmap DrawUtils::ChangeIconColor(QString& _path, QColor& _color)
{
	QColor color = _color;
	// set color value

	// load gray-scale image (an alpha map)
	QPixmap pixmap = QPixmap(_path);

	// initialize painter to draw on a pixmap and set composition mode
	QPainter painter(&pixmap);
	painter.setCompositionMode(QPainter::CompositionMode_SourceIn);

	painter.setBrush(color);
	painter.setPen(color);

	painter.drawRect(pixmap.rect());

	return pixmap;
}

QPixmap DrawUtils::ChangeIconColor(QPixmap _pixmap, QColor& _color)
{
	QColor color = _color;
	// set color value

	// initialize painter to draw on a pixmap and set composition mode
	QPainter painter(&_pixmap);
	painter.setCompositionMode(QPainter::CompositionMode_SourceIn);

	painter.setBrush(color);
	painter.setPen(color);

	painter.drawRect(_pixmap.rect());

	return _pixmap;
}

QString	DrawUtils::ChangeDottedStr(QString& ori, int maxWidth, QFont& font)
{
	QString _text = ori;
	int _width = DrawUtils::GetFontWidth(font, _text);

	if (_width > maxWidth)
	{
		for (int i = _text.length(); i > 0; i--)
		{
			QString newName = _text.mid(0, i);
			newName += "...";
			int newWidth = DrawUtils::GetFontWidth(font, newName);

			if (newWidth < maxWidth)
			{
				_text = newName;
				break;
			}
		}
	}

	return _text;
}

QFont AppUtils::GetRegularFont(int fontSize)
{ 
	QFont returnFont("Noto Sans KR Regular");
	returnFont.setPixelSize(fontSize);

	return returnFont;

}
QFont AppUtils::GetMediumFont(int fontSize)
{ 
	QFont returnFont("Noto Sans KR Medium");
	returnFont.setPixelSize(fontSize);
	
	return returnFont;
}

QFont AppUtils::GetBoldFont(int fontSize)
{
	QFont returnFont("Noto Sans KR Bold");
	returnFont.setPixelSize(fontSize);

	return returnFont;
}