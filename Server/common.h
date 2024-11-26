#ifndef COMMON_H
#define COMMON_H

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>     // printf(), ...
#include <stdlib.h>    // exit(), ...
#include <string.h>    // strncpy(), ...
#include <tchar.h>     // _T(), ...
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32")  // ws2_32.lib
    void
    err_quit(const char* msg);

void err_display(const char* msg);

void err_display(int errcode);

#endif  // COMMON_H