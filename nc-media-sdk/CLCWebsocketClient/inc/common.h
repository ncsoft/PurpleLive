#pragma once

#ifdef CLCWEBSOCKETCLIENT_EXPORTS
#define CLCWEBSOCKETCLIENT_API __declspec(dllexport)
#else
#define CLCWEBSOCKETCLIENT_API __declspec(dllimport)
#endif
