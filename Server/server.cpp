/*** 여기서부터 이 책의 모든 예제에서 공통으로 포함하여 사용하는 코드이다. ***/

#define _CRT_SECURE_NO_WARNINGS  // 구형 C 함수 사용 시 경고 끄기
#define _WINSOCK_DEPRECATED_NO_WARNINGS  // 구형 소켓 API 사용 시 경고 끄기

#include <stdio.h>     // printf(), ...
#include <stdlib.h>    // exit(), ...
#include <string.h>    // strncpy(), ...
#include <tchar.h>     // _T(), ...
#include <winsock2.h>  // 윈속2 메인 헤더
#include <ws2tcpip.h>  // 윈속2 확장 헤더

#pragma comment(lib, "ws2_32")  // ws2_32.lib 링크

// 소켓 함수 오류 출력 후 종료
void err_quit(const char* msg) {
  LPVOID lpMsgBuf;
  FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                 NULL, WSAGetLastError(),
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char*)&lpMsgBuf, 0,
                 NULL);
  MessageBoxA(NULL, (const char*)lpMsgBuf, msg, MB_ICONERROR);
  LocalFree(lpMsgBuf);
  exit(1);
}

// 소켓 함수 오류 출력
void err_display(const char* msg) {
  LPVOID lpMsgBuf;
  FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                 NULL, WSAGetLastError(),
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char*)&lpMsgBuf, 0,
                 NULL);
  printf("[%s] %s\n", msg, (char*)lpMsgBuf);
  LocalFree(lpMsgBuf);
}

// 소켓 함수 오류 출력
void err_display(int errcode) {
  LPVOID lpMsgBuf;
  FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                 NULL, errcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                 (char*)&lpMsgBuf, 0, NULL);
  printf("[오류] %s\n", (char*)lpMsgBuf);
  LocalFree(lpMsgBuf);
}

/*** 여기까지가 이 책의 모든 예제에서 공통으로 포함하여 사용하는 코드이다. ***/
/*** 2장 이후의 예제들은 Common.h를 포함하는 방식으로 이 코드를 사용한다.  ***/

#define SERVERPORT 9000
#define BUFSIZE 512

#include <windows.h>  // windows 관련 함수 포함
#include <iostream>
#include <vector>

CRITICAL_SECTION cs;

typedef struct Player {
  int x, y;
  int dx, dy;
  int jumpSpeed;
  bool isCharging;
  bool isJumping;
  bool isSliding;
  bool slip;  // 미끄러지는 동안 계속 true
  bool damaged;
  std::string face;  // face: left, right
  bool EnhancedJumpPower;
};

typedef struct Item {
  int x, y;
  int interval;
  bool disable;
};

typedef struct Enemy {
  int x, y;
};

typedef struct Bullet {
  int x, y;
  int dx, dy;
};

typedef struct MATCH {
  SOCKET client_sock[2];
  Player player1;
  Player player2;
  char p1, p2;
  int mapNum;
  int score;
  std::vector<Item> g_items;
  std::vector<Enemy> g_enemies;
  std::vector<Bullet> g_bullets;
};
std::vector<MATCH> g_matches;

// 클라이언트와 데이터 통신
DWORD WINAPI RecvProcessClient(LPVOID arg) {
  int retval;
  SOCKET client_sock = (SOCKET)arg;
  // match[메인에서 알려줄 예정].client_socket[메인에서 알려줄예정] = client_sock;
  struct sockaddr_in clientaddr;
  char addr[INET_ADDRSTRLEN];
  int addrlen;
  char buf[BUFSIZE + 1];

  // 클라이언트 정보 얻기
  addrlen = sizeof(clientaddr);
  getpeername(client_sock, (struct sockaddr*)&clientaddr, &addrlen);
  inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));

  EnterCriticalSection(&cs);
  printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n", addr, ntohs(clientaddr.sin_port));
  LeaveCriticalSection(&cs);

  while (1) {
    // 데이터 받기
    retval = recv(client_sock, buf, BUFSIZE, 0);
    if (retval == SOCKET_ERROR) {
      err_display("recv()");
      break;
    } else if (retval == 0)
      break;

    // 받은 데이터 출력
    buf[retval] = '\0';
    EnterCriticalSection(&cs);
    printf("[%s:%d] %s\n", addr, ntohs(clientaddr.sin_port), buf);
    // 메인에서 알려준거에 따라 
    // match[메인에서 알려줌].p? = buf;
    LeaveCriticalSection(&cs);

    if (retval == SOCKET_ERROR) {
      err_display("send()");
      break;
    } 
  }

  // 소켓 닫기
  EnterCriticalSection(&cs);
  closesocket(client_sock);
  printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n", addr, ntohs(clientaddr.sin_port));
  LeaveCriticalSection(&cs);
  return 0;
}

// 방법1. CreateWaitableTimer >> 고정된 짧은 주기와 높은 정확성이 필요한 경우
DWORD WINAPI clientProcess(LPVOID lpParam) {
  // 타이머 생성
  int matchNum; // 메인에서 받을 예정
  HANDLE hTimer = CreateWaitableTimer(NULL, TRUE, NULL);
  if (hTimer == NULL) {
    printf("타이머 생성 실패\n");
    return 1;
  }

  // 타이머 간격을 설정 (1/30초)
  LARGE_INTEGER liDueTime;  // LARGE_INTEGER는 SetWaitableTimer에서 요구함
  liDueTime.QuadPart = -333300;

  while (true) {
    if (!SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, FALSE)) {
      printf("타이머 설정 실패\n");
      CloseHandle(hTimer);
      return 1;
    }

    // 타이머 이벤트가 발생할 때까지 대기
    WaitForSingleObject(hTimer, INFINITE);

    // 매치 데이터 업데이트
    EnterCriticalSection(&cs);
    // 플레이어 좌표 이동
    g_matches[matchNum].player1.x += g_matches[matchNum].player1.dx;
    g_matches[matchNum].player2.x += g_matches[matchNum].player2.dx;
    // 다른 데이터 업데이트 로직 추가 해야함    
    LeaveCriticalSection(&cs);
    //send 추가해야함
    printf("타이머스레드 일함\n");
    // 필요에 따라 타이머 중단 조건을 추가.
  }

  CloseHandle(hTimer);
  return 0;
}
/*
// 방법2. chrono 기반 >> 가독성과 유지보수가 중요하고 플랫폼 독립적이어야 하는 경우
#include <chrono>
#include <thread>
DWORD WINAPI TimerThread(LPVOID arg) {
  const int interval = 1000 / 30;  // 1초에 30번 동작
  while (true) {
    auto start = std::chrono::high_resolution_clock::now();

    // 매치 데이터 업데이트
    EnterCriticalSection(&cs);
    for (size_t i = 0; i < g_matches.size(); ++i) {
      // 플레이어 1의 x 좌표 이동
      g_matches[i].player1.x += g_matches[i].player1.dx;
      g_matches[i].player2.x += g_matches[i].player2.dx;

      // 다른 데이터 업데이트 로직 추가
    }
    LeaveCriticalSection(&cs);

    // 33ms 대기 (1초에 30번)
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::milliseconds elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    if (elapsed.count() < interval) {
      std::this_thread::sleep_for(std::chrono::milliseconds(interval) - elapsed);
    }
  }
  return 0;
}
*/

int main(int argc, char* argv[]) {
  int retval;
  int thread_id = 0;

  // 윈속 초기화
  WSADATA wsa;
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

  // 소켓 생성
  SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_sock == INVALID_SOCKET) err_quit("socket()");

  // bind()
  struct sockaddr_in serveraddr;
  memset(&serveraddr, 0, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons(SERVERPORT);
  retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
  if (retval == SOCKET_ERROR) err_quit("bind()");

  // listen()
  retval = listen(listen_sock, SOMAXCONN);
  if (retval == SOCKET_ERROR) err_quit("listen()");

  // 데이터 통신에 사용할 변수
  SOCKET client_sock;
  struct sockaddr_in clientaddr;
  int addrlen;
  HANDLE hThread;
  InitializeCriticalSection(&cs);
  while (1) {
    // accept()
    addrlen = sizeof(clientaddr);
    client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
    if (client_sock == INVALID_SOCKET) {
      err_display("accept()");
      break;
    }

    // 접속한 클라이언트 정보 출력
    char addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
   // printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n", addr, ntohs(clientaddr.sin_port));

    // 스레드 생성
    hThread = CreateThread(NULL, 0, RecvProcessClient, (LPVOID)client_sock, 0, NULL);
    if (hThread == NULL) {
      closesocket(client_sock);
    } else {
      CloseHandle(hThread);
    }
  }

  // 소켓 닫기
  closesocket(listen_sock);
  DeleteCriticalSection(&cs);
  // 윈속 종료
  WSACleanup();
  return 0;
}