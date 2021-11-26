#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "log.h"

#ifdef _DEBUG
#define DbgOut(x) OutputDebugStringA(x)
#else
#define DbgOut(x)
#endif

static inline void hlogv(const char *format, va_list args)
{
	char message[1024] = "";	
	int num = _vsprintf_p(message, 1024, format, args);
	message[num] = '\n';
	message[num+1] = 0;
	DbgOut(message);
}

void hlog(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	hlogv(format, args);
	va_end(args);
}

void hlog_hr(const char *text, HRESULT hr)
{
	LPSTR buffer = NULL;

	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
			       FORMAT_MESSAGE_ALLOCATE_BUFFER |
			       FORMAT_MESSAGE_IGNORE_INSERTS,
		       NULL, hr, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		       (LPSTR)&buffer, 0, NULL);

	if (buffer) {
		hlog("%s (0x%08lX): %s", text, hr, buffer);
		LocalFree(buffer);
	} else {
		hlog("%s (0x%08lX)", text, hr);
	}
}