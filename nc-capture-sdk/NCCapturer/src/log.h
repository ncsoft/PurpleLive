#pragma once

#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

void hlog(const char *format, ...);
void hlog_hr(const char *text, HRESULT hr);

#ifdef __cplusplus
}
#endif
