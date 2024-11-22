/*** ���⼭���� �� å�� ��� �������� �������� �����Ͽ� ����ϴ� �ڵ��̴�. ***/

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

// ���� �Լ� ���� ���
void err_display(const char* msg) {
  LPVOID lpMsgBuf;
  FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                 NULL, WSAGetLastError(),
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char*)&lpMsgBuf, 0,
                 NULL);
  printf("[%s] %s\n", msg, (char*)lpMsgBuf);
  LocalFree(lpMsgBuf);
}

// ���� �Լ� ���� ���
void err_display(int errcode) {
  LPVOID lpMsgBuf;
  FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                 NULL, errcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                 (char*)&lpMsgBuf, 0, NULL);
  printf("[����] %s\n", (char*)lpMsgBuf);
  LocalFree(lpMsgBuf);
}

/*** ��������� �� å�� ��� �������� �������� �����Ͽ� ����ϴ� �ڵ��̴�. ***/
/*** 2�� ������ �������� Common.h�� �����ϴ� ������� �� �ڵ带 ����Ѵ�.  ***/

#define SERVERPORT 9000
#define BUFSIZE 512

#include <windows.h>  // windows ���� �Լ� ����
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
  bool slip;  // �̲������� ���� ��� true
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

// Ŭ���̾�Ʈ�� ������ ���
DWORD WINAPI RecvProcessClient(LPVOID arg) {
  recvParam* param = (recvParam*)arg;
  SOCKET client_sock = param->client_sock;
  char matchNum = param->matchNum;
  char playerNum = param->playerNum;
  delete param;  // ���� �Ҵ� ����

  int retval;
  struct sockaddr_in clientaddr;
  char addr[INET_ADDRSTRLEN];
  int addrlen;
  char buf[BUFSIZE + 1];

  // Ŭ���̾�Ʈ ���� ���
  addrlen = sizeof(clientaddr);
  getpeername(client_sock, (struct sockaddr*)&clientaddr, &addrlen);
  inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));

  printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", addr, ntohs(clientaddr.sin_port));

  while (1) {
    // ������ �ޱ�
    retval = recv(client_sock, buf, BUFSIZE, 0);
    if (retval == SOCKET_ERROR) {
      err_display("recv()");
      break;
    } else if (retval == 0)
      break;

    // ���� ������ ���
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

  // ���� �ݱ�
  EnterCriticalSection(&cs);
  closesocket(client_sock);
  printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", addr, ntohs(clientaddr.sin_port));
  LeaveCriticalSection(&cs);
  return 0;
}

void updatePlayer(int matchNum) {
  // player1 ó��

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
    // ����, ������ Ű�� ��� ������ ���� ����
    if (g_matches[matchNum].player1.dx > 0) {
      g_matches[matchNum].player1.dx -= 1;
    } else if (g_matches[matchNum].player1.dx < 0) {
      g_matches[matchNum].player1.dx += 1;
    }
  }
  g_matches[matchNum].player1.x += g_matches[matchNum].player1.dx;
  g_matches[matchNum].p1 = 'a';
  // player2 ó��
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
    // ����, ������ Ű�� ��� ������ ���� ����
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
  // Ÿ�̸� ����
  int matchNum = *(int*)lpParam;
  delete (int*)lpParam;
  HANDLE hTimer = CreateWaitableTimer(NULL, TRUE, NULL);
  if (hTimer == NULL) {
    printf("Ÿ�̸� ���� ����\n");
    return 1;
  }

  // Ÿ�̸� ������ ���� (1/30��)
  LARGE_INTEGER liDueTime;  // LARGE_INTEGER�� SetWaitableTimer���� �䱸��
  liDueTime.QuadPart = -333300;

  while (true) {
    if (!SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, FALSE)) {
      printf("Ÿ�̸� ���� ����\n");
      CloseHandle(hTimer);
      return 1;
    }

    // Ÿ�̸� �̺�Ʈ�� �߻��� ������ ���
    WaitForSingleObject(hTimer, INFINITE);

    // ��ġ ������ ������Ʈ
    EnterCriticalSection(&cs);
    // �÷��̾� ��ǥ �̵�   
    updatePlayer(matchNum);
     printf("%d, %d\r", g_matches[matchNum].player1.dx, g_matches[matchNum].player2.dx);
    // �ٸ� ������ ������Ʈ ���� �߰� �ؾ���    
    // send �κ�
    char sendBuf[BUFSIZE];
    int sendSize = snprintf(sendBuf, BUFSIZE, "P1_X:%d,P2_X:%d",
                 g_matches[matchNum].player1.x, g_matches[matchNum].player2.x);

    for (int i = 0; i < 2; ++i) {
      if (g_matches[matchNum].client_sock[i] == NULL) {
        //printf("Ŭ���̾�Ʈ %d ������ NULL�Դϴ�.\n", i);
        continue;
      }
      //printf("Ŭ���̾�Ʈ %d ���� Ȯ��: %d\n", i, g_matches[matchNum].client_sock[i]);
      int retval = send(g_matches[matchNum].client_sock[i], sendBuf, sendSize, 0);
      if (retval == SOCKET_ERROR) {
        printf("Ŭ���̾�Ʈ %d���� ������ ���� ����: %d\n", i,
               WSAGetLastError());
      } else {
        //printf("Ŭ���̾�Ʈ %d���� ������ ���� ����: %d ����Ʈ ���۵�\n", i, retval);
      }
    }
    LeaveCriticalSection(&cs);
    // �ʿ信 ���� Ÿ�̸� �ߴ� ������ �߰�.
  }

  CloseHandle(hTimer);
  return 0;
}

int main(int argc, char* argv[]) {
  int retval;

  // ���� �ʱ�ȭ
  WSADATA wsa;
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

  // ���� ����
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

  // ������ ��ſ� ����� ����
  struct sockaddr_in clientaddr;
  int addrlen;
  HANDLE hThread;
  recvParam *rParam;
  InitializeCriticalSection(&cs);

  while (1) {
    printf("���� �����...\n");
    addrlen = sizeof(clientaddr);
    rParam = new recvParam{};
    rParam->client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
    if (rParam->client_sock == INVALID_SOCKET) {
      err_display("accept()");
      break;
    }
    // clinetCount: ���� Ŭ���̾�Ʈ ��, 2��° Ŭ���̾�Ʈ�� ��ġ�� �߰��Ǹ� �ٽ� 0, 
    // ��ġ ������ ��ġ �� �÷��̾� �� �Ǵ� matchCount: ���� ��ġ ��, 
    // 2��° Ŭ���̾�Ʈ�� ��ġ�� �߰��Ǹ� +1, ��ġ ��ȣ�� ���� ��ġ�� �������� matchNum ���� �ʿ�, 
    // Ŭ���̾�Ʈ ���� ���� �� recv �ۼ��� Ȯ���ϰ� �߰��� ����
    char addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
    printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", addr,
           ntohs(clientaddr.sin_port));

    if (clientCount == 0) g_matches.push_back(MATCH());
    // ��ġ ����ü �ʱ�ȭ �ؾ��� 
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

  // ���� �ݱ�
  closesocket(listen_sock);
  DeleteCriticalSection(&cs);
  // ���� ����
  WSACleanup();
  return 0;
}