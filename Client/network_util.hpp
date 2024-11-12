#pragma once

#ifndef NETWORK_UTIL
#define NETWORK_UTIL
#define WIN32_LEAN_AND_MEAN
#include <WS2tcpip.h>
#include <stdio.h>
#include <stdlib.h>

#include <print>
#pragma comment(lib, "ws2_32")

// 소켓 함수 오류 출력 후 종료
void err_quit(const char *msg) {
  LPVOID lpMsgBuf;

  FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                 NULL, WSAGetLastError(),
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char *)&lpMsgBuf,
                 0, NULL);
  MessageBoxA(NULL, (const char *)lpMsgBuf, msg, MB_ICONERROR);
  LocalFree(lpMsgBuf);
  exit(1);
}

// 소켓 함수 오류 출력
void err_display(const char *msg) {
  LPVOID lpMsgBuf;
  FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                 NULL, WSAGetLastError(),
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char *)&lpMsgBuf,
                 0, NULL);
  std::println("[{}] {}", msg, (char const *)lpMsgBuf);
  LocalFree(lpMsgBuf);
}

// 소켓 함수 오류 출력
void err_display(int errcode) {
  LPVOID lpMsgBuf;
  FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                 NULL, errcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                 (char *)&lpMsgBuf, 0, NULL);
  std::println("[오류] {}", (char *)lpMsgBuf);
  LocalFree(lpMsgBuf);
}

#endif