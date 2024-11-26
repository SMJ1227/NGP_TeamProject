#ifndef COMMON_H
#define COMMON_H

#define _CRT_SECURE_NO_WARNINGS  // ���� C �Լ� ��� �� ��� ����
#define _WINSOCK_DEPRECATED_NO_WARNINGS  // ���� ���� API ��� �� ��� ����

#include <stdio.h>     // printf(), ...
#include <stdlib.h>    // exit(), ...
#include <string.h>    // strncpy(), ...
#include <tchar.h>     // _T(), ...
#include <winsock2.h>  // ����2 ���� ���
#include <ws2tcpip.h>  // ����2 Ȯ�� ���

#pragma comment(lib, "ws2_32")  // ws2_32.lib ��ũ

// ���� �Լ� ���� ��� �� ����
void err_quit(const char* msg);

// ���� �Լ� ���� ���
void err_display(const char* msg);

// ���� �Լ� ���� ���
void err_display(int errcode);

#endif  // COMMON_H
