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

char clientCount = 0;
char matchCount = 0;
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

struct recvParam {
  SOCKET client_sock = NULL;
  char matchNum = NULL;
  char playerNum = NULL;
};

typedef struct MATCH {
  SOCKET client_sock[2]{NULL, NULL};
  HANDLE recvThread[2]{NULL, NULL};
  HANDLE timerThread;
  Player player1;
  Player player2;
  char p1 = 'a';
  char p2 = 'a';
  int mapNum;
  int score;
  std::vector<Item> g_items;
  std::vector<Enemy> g_enemies;
  std::vector<Bullet> g_bullets;
};
std::vector<MATCH> g_matches;

// 클라이언트와 데이터 통신
DWORD WINAPI RecvProcessClient(LPVOID arg) {
  recvParam* param = (recvParam*)arg;
  SOCKET client_sock = param->client_sock;
  char matchNum = param->matchNum;
  char playerNum = param->playerNum;
  delete param;  // 동적 할당 해제

  int retval;
  struct sockaddr_in clientaddr;
  char addr[INET_ADDRSTRLEN];
  int addrlen;
  char buf[BUFSIZE + 1];

  // 클라이언트 정보 얻기
  addrlen = sizeof(clientaddr);
  getpeername(client_sock, (struct sockaddr*)&clientaddr, &addrlen);
  inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));

  printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n", addr, ntohs(clientaddr.sin_port));

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
    
    if (playerNum == 0) {
      g_matches[matchNum].p1 = buf[0];
      EnterCriticalSection(&cs);
      // printf("[%s:%d] %c\n", addr, ntohs(clientaddr.sin_port), g_matches[matchNum].p1);
      LeaveCriticalSection(&cs);
    }
    if (playerNum == 1) {
      g_matches[matchNum].p2 = buf[0];
      EnterCriticalSection(&cs);
      printf("[%s:%d] %c\n", addr, ntohs(clientaddr.sin_port), g_matches[matchNum].p2);
      LeaveCriticalSection(&cs);
    }

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

void updatePlayer(int matchNum) {
  // player1 처리

  //printf("%c\r", g_matches[matchNum].p1);
  if (g_matches[matchNum].p1 == '0') {
    if (g_matches[matchNum].player1.dx >= -3) {
      g_matches[matchNum].player1.dx -= 1;
    }
  } 
  else if (g_matches[matchNum].p1 == '1') {
    if (g_matches[matchNum].player1.dx <= 3) {
      g_matches[matchNum].player1.dx += 1;
    }
  } 
  else if (g_matches[matchNum].p1 != '0' && g_matches[matchNum].p1 != '1') {
    // 왼쪽, 오른쪽 키가 모두 눌리지 않은 상태
    if (g_matches[matchNum].player1.dx > 0) {
      g_matches[matchNum].player1.dx -= 1;
    } else if (g_matches[matchNum].player1.dx < 0) {
      g_matches[matchNum].player1.dx += 1;
    }
  }
  g_matches[matchNum].player1.x += g_matches[matchNum].player1.dx;
  g_matches[matchNum].p1 = 'a';
  // player2 처리
  /*
  if (g_matches[matchNum].p2 == 0) {
    if (g_matches[matchNum].player2.dx >= -3) {
      g_matches[matchNum].player2.dx -= 1;
    }
  } 
  else if (g_matches[matchNum].p2 == 1) {
    if (g_matches[matchNum].player2.dx <= 3) {
      g_matches[matchNum].player2.dx += 1;
    }
  } 
  else if (g_matches[matchNum].p2 != 0 && g_matches[matchNum].p2 != 1) {
    // 왼쪽, 오른쪽 키가 모두 눌리지 않은 상태
    if (g_matches[matchNum].player2.dx > 0) {
      g_matches[matchNum].player2.dx -= 1;
    } else if (g_matches[matchNum].player2.dx < 0) {
      g_matches[matchNum].player2.dx += 1;
    }
  }
  g_matches[matchNum].player2.x += g_matches[matchNum].player2.dx;
  */
}

DWORD WINAPI timerProcessClient(LPVOID lpParam) {
  // 타이머 생성
  int matchNum = *(int*)lpParam;
  delete (int*)lpParam;
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
    updatePlayer(matchNum);
     printf("%d, %d\r", g_matches[matchNum].player1.dx, g_matches[matchNum].player2.dx);
    // 다른 데이터 업데이트 로직 추가 해야함    
    // send 부분
    char sendBuf[BUFSIZE];
    int sendSize = snprintf(sendBuf, BUFSIZE, "P1_X:%d,P2_X:%d",
                 g_matches[matchNum].player1.x, g_matches[matchNum].player2.x);

    for (int i = 0; i < 2; ++i) {
      if (g_matches[matchNum].client_sock[i] == NULL) {
        //printf("클라이언트 %d 소켓이 NULL입니다.\n", i);
        continue;
      }
      //printf("클라이언트 %d 소켓 확인: %d\n", i, g_matches[matchNum].client_sock[i]);
      int retval = send(g_matches[matchNum].client_sock[i], sendBuf, sendSize, 0);
      if (retval == SOCKET_ERROR) {
        printf("클라이언트 %d에게 데이터 전송 실패: %d\n", i,
               WSAGetLastError());
      } else {
        //printf("클라이언트 %d에게 데이터 전송 성공: %d 바이트 전송됨\n", i, retval);
      }
    }
    LeaveCriticalSection(&cs);
    // 필요에 따라 타이머 중단 조건을 추가.
  }

  CloseHandle(hTimer);
  return 0;
}

int main(int argc, char* argv[]) {
  int retval;

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
  struct sockaddr_in clientaddr;
  int addrlen;
  HANDLE hThread;
  recvParam *rParam;
  InitializeCriticalSection(&cs);

  while (1) {
    printf("서버 대기중...\n");
    addrlen = sizeof(clientaddr);
    rParam = new recvParam{};
    rParam->client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
    if (rParam->client_sock == INVALID_SOCKET) {
      err_display("accept()");
      break;
    }
    // clinetCount: 현재 클라이언트 수, 2번째 클라이언트가 매치에 추가되면 다시 0, 
    // 매치 생성과 매치 내 플레이어 수 판단 matchCount: 현재 매치 수, 
    // 2번째 클라이언트가 매치에 추가되면 +1, 매치 번호로 전달 매치가 없어지면 matchNum 조정 필요, 
    // 클라이언트 메인 만든 후 recv 송수신 확인하고 추가할 예정
    char addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
    printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n", addr,
           ntohs(clientaddr.sin_port));

    if (clientCount == 0) g_matches.push_back(MATCH());
    // 매치 구조체 초기화 해야함 
    // 
    if (g_matches[matchCount].client_sock[clientCount] == NULL) {
      rParam->playerNum = 0;
      rParam->matchNum = matchCount;
      g_matches[matchCount].client_sock[clientCount] = rParam->client_sock;
      g_matches[matchCount].recvThread[0] = CreateThread(NULL, 0, RecvProcessClient, rParam, 0, NULL);
      clientCount++;
      int* pMatchNum = new int(matchCount);
      g_matches[matchCount].timerThread = CreateThread(NULL, 0, timerProcessClient, pMatchNum, 0, NULL);
    } 
    else if (g_matches[matchCount].client_sock[0] != NULL &&
               g_matches[matchCount].client_sock[1] == NULL) {
      rParam->playerNum = 1;
      rParam->matchNum = matchCount;
      g_matches[matchCount].recvThread[1] =
          CreateThread(NULL, 0, RecvProcessClient, rParam, 0, NULL);
      matchCount++;
      clientCount = 0;
    }
  }

  // 소켓 닫기
  closesocket(listen_sock);
  DeleteCriticalSection(&cs);
  // 윈속 종료
  WSACleanup();
  return 0;
}