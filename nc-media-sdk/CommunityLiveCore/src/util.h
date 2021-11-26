#pragma once

#include <string>
#include <windows.h>

static std::string format_string(const char *format, ...)
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
