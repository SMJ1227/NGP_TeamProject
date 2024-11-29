#define SERVERPORT 9000
#define BUFSIZE 512

#include "common.h"
#include <windows.h>  // windows ���� �Լ� ����

#include <iostream>
#include <vector>

#include "map.h"
#include "sendParam.hpp"

CRITICAL_SECTION cs;
HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

typedef struct Player {
  int x, y;
  int dx = 0, dy = 0;
  int jumpSpeed = 0;
  bool isCharging = false;
  bool isJumping = false;
  bool isSliding = false;
  bool slip = false;  // �̲������� ���� ��� true
  bool damaged = false;
  std::string face = "left";  // face: left, right
  bool EnhancedJumpPower = false;
  bool spaceKeyReleased = true;
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
  sendParam::sendParam SPlayer1;
  Player player2;
  sendParam::sendParam SPlayer2;
  int map[MAP_HEIGHT][MAP_WIDTH];
  char matchNum = 0;
  char p1 = 'a';
  char p2 = 'a';
  int mapNum = 1;
  int score = 0;
  std::vector<Item> g_items;
  std::vector<Enemy> g_enemies;
  std::vector<Bullet> g_bullets;

  bool operator==(const MATCH& other) {
    return client_sock[0] == other.client_sock[0] &&
           client_sock[1] == other.client_sock[1];
  }
};
std::vector<MATCH> g_matches;

// �ʱ�ȭ �Լ�
void InitMap(int matchNum, int src[MAP_HEIGHT][MAP_WIDTH]);
void initPlayer(int matchNum);
void initItem(int matchNum);
void GenerateItem(int matchNum, int x, int y);
void DeleteAllItems(int matchNum);
void initEnemy(int matchNum);
void GenerateEnemy(int matchNum, int x, int y);
void DeleteAllEnemies(int matchNum);
void ShootBullet(int matchNum);
void DeleteAllBullets(int matchNum);
void initAll(int matchNum);
// �̵� �Լ�
void updatePlayerD(int matchNum);
void applyGravity(int matchNum);
bool IsColliding(int matchNum, int x, int y);
bool IsSlopeGoRightColliding(int matchNum, int x, int y);
bool IsSlopeGoLeftColliding(int matchNum, int x, int y);
bool IsNextColliding(int matchNum, int x, int y);
void movePlayer(int matchNum);
void moveBullets(int matchNum);
// ������Ʈ �浹ó��
void CheckCollisions(int matchNum);
void CheckEnemyPlayerCollisions(int matchNum);
void CheckItemPlayerCollisions(int matchNum);
void CheckPlayerBulletCollisions(int matchNum);
void CheckPlayersCollisions(int matchNum);
void updateSendParam(int matchNum);

// ��ġ�� �����ϴ� �Լ�
void closeSocketFunc(SOCKET client_sock, char matchNum, char playerNum) {
  // ����׿� ���
  printf("%d��ġ�� %d�� �÷��̾��� ������ ����ƽ��ϴ�\n", matchNum, playerNum);

  std::vector<MATCH>::iterator iter = g_matches.begin();
  for (int i = 0; i < matchNum; i++) iter++;
  g_matches.erase(std::remove(g_matches.begin(), g_matches.end(), *iter),
                  g_matches.end());
  // ����׿� ���
  printf("��ġ ���� ����\n");

  for (int i = 0; i < g_matches.size(); i++) {
    g_matches[i].matchNum = i;
  }

  // �ٲ� matchNum�� �ٸ� �����忡�� ������Ѿ���
  // -> �� recv�����忡�� �ݺ����� ���۵� �� �ڽ��� ��ġ ��ȣ�� �˻��Ѵ�
}

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

  printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", addr,
         ntohs(clientaddr.sin_port));

  while (1) {
    // �ٸ� ������ �۵��� ���
    WaitForSingleObject(hEvent, INFINITE);
    ResetEvent(hEvent);
    // ������ ��ȿ�� ���� ������, ���� ��ġ�� ��ȣ��, ��ġ[���� ��ġ��ȣ]�� ��ġ
    // ��ȣ�� ��ġ�ϴ��� ���Ѵ�, �ٸ��ٸ� ����

    while (matchNum >= 0 && matchNum < g_matches.size() &&
           g_matches[matchNum].matchNum != matchNum) {
      // ����׿� ���
      printf(
          "matchNum: %d\n matchNum��° ��ġ�� ���� ��ġ ��ȣ: %d\n��ġ���� "
          "����, matchNum����\n",
          matchNum, g_matches[matchNum].matchNum);
      matchNum--;
    }
    // ����׿� ���
    // printf("\nrecvThread ���� ����, matchNum: %d, playerNum: %d\n", matchNum,
    // playerNum); ������ �ޱ�
    retval = recv(client_sock, buf, BUFSIZE, 0);
    if (retval == SOCKET_ERROR) {
      err_display("recv()");
      closeSocketFunc(client_sock, matchNum, playerNum);
      SetEvent(hEvent);
      break;
    } else if (retval == 0) {
      SetEvent(hEvent);
      break;
    }

    // ���� ������ ���
    buf[retval] = '\0';
    if (playerNum == 0) {
      g_matches[matchNum].p1 = buf[0];
      // printf("%d��ġ %d�÷��̾�� ���� ������: %c\n", matchNum, playerNum,
      // buf[0]);    // ���� 0, ������ 1, �����̽� �Է¶� �ѹ�, ���� �ѹ�
    }
    if (playerNum == 1) {
      g_matches[matchNum].p2 = buf[0];
      printf("[%s:%d] %c\n", addr, ntohs(clientaddr.sin_port),
             g_matches[matchNum].p2);
    }

    if (retval == SOCKET_ERROR) {
      err_display("send()");
      SetEvent(hEvent);
      break;
    }
    // �̺�Ʈ ����
    SetEvent(hEvent);
  }
  // ���� �ݱ�
  closesocket(client_sock);
  printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", addr,
         ntohs(clientaddr.sin_port));
  return 0;
}

DWORD WINAPI timerProcessClient(LPVOID lpParam) {
  // Ÿ�̸� ����
  int matchNum = (*(int*)lpParam);
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
    // Ÿ�̸� ����
    if (!SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, FALSE)) {
      printf("Ÿ�̸� ���� ����\n");
      CloseHandle(hTimer);
      SetEvent(hEvent);
      return 1;
    }

    // Ÿ�̸� ��� ����
    // printf("Ÿ�̸� ��� ����\n");
    DWORD waitResult = WaitForSingleObject(hTimer, INFINITE);
    if (waitResult != WAIT_OBJECT_0) {
      printf("Ÿ�̸� ��� ���� �Ǵ� �ߴ�: %d\n", GetLastError());
      break;
    }
    // printf("Ÿ�̸� ��� �Ϸ�\n");

    //// �̺�Ʈ ���
    // WaitForSingleObject(hEvent, INFINITE);
    // ResetEvent(hEvent);

    // ������ ��ȿ�� ���� ������, ���� ��ġ ��ȣ��, ��ġ[���� ��ġ��ȣ]�� ��ġ
    // ��ȣ�� ��ġ�ϴ� �� ���Ѵ�, �ٸ��ٸ� ����
    while (!(matchNum < 0) && matchNum < g_matches.size() &&
           g_matches[matchNum].matchNum != matchNum) {
      // ����׿� ���
      printf(
          "matchNum: %d\n matchNum��° ��ġ�� ���� ��ġ ��ȣ: %d\n��ġ���� "
          "����, matchNum����\n",
          matchNum, g_matches[matchNum].matchNum);
      matchNum--;
    }

    EnterCriticalSection(&cs);
    // �÷��̾� dx dy ��ȭ
    updatePlayerD(matchNum);
    // printf("%d, %d\n", g_matches[matchNum].player1.dx,
    // g_matches[matchNum].player1.dy);
    applyGravity(matchNum);
    movePlayer(matchNum);
    // moveBullets();
    // shootInterval++
    // ������ ����� �ڵ�
    // �Ѿ� ����� �ڵ�
    // ��Ż �浹ó��
    // ������Ʈ �浹ó��
    // sendParam������Ʈ
    // printf("match %d: %d, %d\r", matchNum, g_matches[matchNum].SPlayer1.x,
    // g_matches[matchNum].SPlayer1.y);
    updateSendParam(matchNum);
    LeaveCriticalSection(&cs);
    // send �κ�
    char sendBuf[1 + BUFSIZE];
    int sendSize = 1 + sizeof(sendParam::sendParam);

    for (int i = 0; i < 2; ++i) {
      if (g_matches[matchNum].client_sock[i] == NULL) {
        // printf("Ŭ���̾�Ʈ %d ������ NULL�Դϴ�.\n", i);
        continue;
      }
      // printf("Ŭ���̾�Ʈ %d ���� Ȯ��: %d\n", i,
      // g_matches[matchNum].client_sock[i]);
      if (i == 0) {
        sendBuf[0] = static_cast<std::int8_t>(
            sendParam::PKT_CAT::PLAYER_INFO);  // ��Ŷ Ÿ�� ����
        memcpy(sendBuf + 1, &g_matches[matchNum].SPlayer1,
               sizeof(sendParam::sendParam));  // ������ ����
      } else if (i == 1) {
        sendBuf[0] = static_cast<std::int8_t>(
            sendParam::PKT_CAT::PLAYER_INFO);  // ��Ŷ Ÿ�� ����
        memcpy(sendBuf + 1, &g_matches[matchNum].SPlayer2,
               sizeof(sendParam::sendParam));  // ������ ����
      }
      int retval =
          send(g_matches[matchNum].client_sock[i], sendBuf, sendSize, 0);
      if (retval == SOCKET_ERROR) {
        printf("Ŭ���̾�Ʈ %d���� ������ ���� ����: %d\n", i,
               WSAGetLastError());
      } else {
        // printf("Ŭ���̾�Ʈ %d���� ������ ���� ����: %d ����Ʈ ���۵�\n", i,
        // retval);
      }
    }
    // �̺�Ʈ ����
    SetEvent(hEvent);
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

  DWORD opt_val = 1;
  setsockopt(listen_sock, IPPROTO_TCP, TCP_NODELAY, (char const*)&opt_val,
             sizeof(opt_val));

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
  recvParam* rParam;
  int* matchNumParam;
  InitializeCriticalSection(&cs);

  while (1) {
    printf("���� �����...\n");
    addrlen = sizeof(clientaddr);
    // ���⼭ rParam �Ҵ� �ؼ� ���� �ϰ� rParam�� �ְ�
    rParam = new recvParam{};
    matchNumParam = new int{};
    rParam->client_sock =
        accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
    if (rParam->client_sock == INVALID_SOCKET) {
      err_display("accept()");
      break;
    }
    // ��ġ ���� ���� - ���� ��ġ�� ���ų�(0), ������ ��ġ�� player�� ��
    // �������� ���� �÷��̾� 1 ���� ����: ������ ��ġ�� ����0���� ������� ����
    // �÷��̾� 2 ���� ����: ������ ��ġ�� ����1�� ���ְ� ����2�� ������� ����
    // Ÿ�̸� ���� ����: �÷��̾� 1 ������ ��
    char addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
    /*printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", addr,
           ntohs(clientaddr.sin_port));*/
    // ��ġ ����
    if (g_matches.size() == 0 ||
        (!g_matches.empty() && g_matches.back().client_sock[0] != NULL &&
         g_matches.back().client_sock[1] != NULL))
      g_matches.push_back(MATCH());
    // �÷��̾�1 ������, Ÿ�̸� ������ ����
    if (g_matches.back().client_sock[0] == NULL) {
      rParam->playerNum = 0;
      rParam->matchNum = g_matches.size() - 1;
      *matchNumParam = g_matches.size() - 1;
      // g_matches�� Ŭ���̾�Ʈ ����, ��ġ �ѹ� ������Ʈ
      g_matches.back().client_sock[0] = rParam->client_sock;
      g_matches.back().matchNum = g_matches.size() - 1;
      initAll(*matchNumParam);
      // ���� ������ ����
      g_matches.back().recvThread[0] =
          CreateThread(NULL, 0, RecvProcessClient, rParam, 0, NULL);
      // ����׿� ���
      printf("%zu�� ��ġ �����.. Ŭ���̾�Ʈ ��: %d\n", g_matches.size() - 1,
             1);
      // Ÿ�̸� ������ ����
      g_matches.back().timerThread =
          CreateThread(NULL, 0, timerProcessClient, matchNumParam, 0, NULL);
    }
    // �÷��̾�2 ������ ����

    else if (g_matches.back().client_sock[0] != NULL &&
             g_matches.back().client_sock[1] == NULL) {
      printf("2�� �ݺ��� ����\n");
      rParam->playerNum = 1;
      rParam->matchNum = g_matches.size() - 1;
      g_matches.back().client_sock[1] = rParam->client_sock;
      // ����׿� ���
      printf("%d��° ��ġ %d��° �÷��̾� ������ ����\n", rParam->matchNum,
             rParam->playerNum);
      g_matches.back().recvThread[1] =
          CreateThread(NULL, 0, RecvProcessClient, rParam, 0, NULL);
      initPlayer(*matchNumParam);
    }
    // �̺�Ʈ ����
    SetEvent(hEvent);
  }

  // ���� �ݱ�
  closesocket(listen_sock);
  DeleteCriticalSection(&cs);
  // ���� ����
  WSACleanup();
  return 0;
}

void InitMap(int matchNum, int src[MAP_HEIGHT][MAP_WIDTH]) {
  for (int i = 0; i < MAP_HEIGHT; i++) {
    for (int j = 0; j < MAP_WIDTH; j++) {
      g_matches[matchNum].map[i][j] = src[i][j];
    }
  }
}

void initPlayer(int matchNum) {
  g_matches[matchNum].player1.x = (MAP_WIDTH - 7) * GRID;
  g_matches[matchNum].player1.y = (MAP_HEIGHT - 4) * GRID;

  g_matches[matchNum].player2.x = (MAP_WIDTH - 8) * GRID;
  g_matches[matchNum].player2.y = (MAP_HEIGHT - 4) * GRID;
}

// ������
void initItem(int matchNum) {
  for (int y = 0; y < MAP_HEIGHT; y++) {
    for (int x = 0; x < MAP_WIDTH; x++) {
      if (g_matches[matchNum].map[y][x] == 5) {
        GenerateItem(matchNum, x, y);
      }
    }
  }
}

void GenerateItem(int matchNum, int x, int y) {
  Item newItem;
  newItem.x = x;
  newItem.y = y;
  newItem.interval = 1;
  newItem.disable = true;
  g_matches[matchNum].g_items.push_back(newItem);
}

void DeleteAllItems(int matchNum) { g_matches[matchNum].g_items.clear(); }

void initEnemy(int matchNum) {
  for (int y = 0; y < MAP_HEIGHT; y++) {
    for (int x = 0; x < MAP_WIDTH; x++) {
      if (g_matches[matchNum].map[y][x] == 4) {  // ��
        GenerateEnemy(matchNum, x, y);
      }
    }
  }
}

void GenerateEnemy(int matchNum, int x, int y) {
  Enemy newEnemy;
  newEnemy.x = x;
  newEnemy.y = y;
  g_matches[matchNum].g_enemies.push_back(newEnemy);
}

void DeleteAllEnemies(int matchNum) { g_matches[matchNum].g_enemies.clear(); }

void ShootBullet(int matchNum) {
  for (const auto& enemy : g_matches[matchNum].g_enemies) {
    Bullet newBullet;
    newBullet.x = (enemy.x + 1) * GRID;  // ���� ��ġ���� �Ѿ��� �������� ����
    newBullet.y = enemy.y * GRID + GRID / 2;
    newBullet.dx = 2;
    newBullet.dy = 0;
    g_matches[matchNum].g_bullets.push_back(newBullet);
  }
}

void DeleteAllBullets(int matchNum) { g_matches[matchNum].g_bullets.clear(); }

void initAll(int matchNum) {
  InitMap(matchNum, map1);
  initPlayer(matchNum);
  initItem(matchNum);
  initEnemy(matchNum);
}

void updatePlayerD(
    int matchNum) {  // a�� �ٲ�����⶧���� �ѹ��� dx������ �̷������.
  // player1 ó��
  if (g_matches[matchNum].p1 == '0') {
    if (g_matches[matchNum].player1.dx >= -3) {
      g_matches[matchNum].player1.dx -= 1;
    }
  } else if (g_matches[matchNum].p1 == '1') {
    if (g_matches[matchNum].player1.dx <= 3) {
      g_matches[matchNum].player1.dx += 1;
    }
  } else if (g_matches[matchNum].p1 != '0' && g_matches[matchNum].p1 != '1') {
    // ����, ������ Ű�� ��� ������ ���� ����
    if (g_matches[matchNum].player1.dx > 0) {
      g_matches[matchNum].player1.dx -= 1;
    } else if (g_matches[matchNum].player1.dx < 0) {
      g_matches[matchNum].player1.dx += 1;
    }
  }
  g_matches[matchNum].p1 = 'a';
  // player2 ó��
  if (g_matches[matchNum].p2 == 0) {
    if (g_matches[matchNum].player2.dx >= -3) {
      g_matches[matchNum].player2.dx -= 1;
    }
  } else if (g_matches[matchNum].p2 == 1) {
    if (g_matches[matchNum].player2.dx <= 3) {
      g_matches[matchNum].player2.dx += 1;
    }
  } else if (g_matches[matchNum].p2 != 0 && g_matches[matchNum].p2 != 1) {
    // ����, ������ Ű�� ��� ������ ���� ����
    if (g_matches[matchNum].player2.dx > 0) {
      g_matches[matchNum].player2.dx -= 1;
    } else if (g_matches[matchNum].player2.dx < 0) {
      g_matches[matchNum].player2.dx += 1;
    }
  }
  g_matches[matchNum].p2 = 'a';
}

void applyGravity(int matchNum) {
  if (g_matches[matchNum].player1.dy < 20) {
    g_matches[matchNum].player1.dy += GRAVITY;  // �߷� ����
  }
  if (g_matches[matchNum].player2.dy < 20) {
    g_matches[matchNum].player2.dy += GRAVITY;  // �߷� ����
  }
}

bool IsColliding(int matchNum, int x, int y) {
  int gridX = x / GRID;
  int gridY = y / GRID;

  if (gridX < 0 || gridX >= MAP_WIDTH || gridY < 0 || gridY >= MAP_HEIGHT) {
    return true;
  }

  if (g_matches[matchNum].map[gridY][gridX] == 0) {
    return true;
  }
  return false;
}

bool IsSlopeGoRightColliding(int matchNum, int x, int y) {
  int gridX = x / GRID;
  int gridY = y / GRID;
  int leftX = (x - PLAYER_SIZE / 2) / GRID;
  int rightX = (x + PLAYER_SIZE / 2 - 1) / GRID;
  int topY = (y - PLAYER_SIZE / 2) / GRID;
  int bottomY = (y + PLAYER_SIZE / 2 - 1) / GRID;

  // �浹 ����
  if (g_matches[matchNum].map[bottomY][leftX] == 2 ||
      g_matches[matchNum].map[bottomY][rightX] == 2) {
    return true;
  }
  if (g_matches[matchNum].map[gridY][gridX] == 2) {
    return true;
  }
  return false;
}

bool IsSlopeGoLeftColliding(int matchNum, int x, int y) {
  int gridX = x / GRID;
  int gridY = y / GRID;
  int leftX = (x - PLAYER_SIZE / 2) / GRID;
  int rightX = (x + PLAYER_SIZE / 2 - 1) / GRID;
  int topY = (y - PLAYER_SIZE / 2) / GRID;
  int bottomY = (y + PLAYER_SIZE / 2 - 1) / GRID;

  // �浹 ����
  if (g_matches[matchNum].map[bottomY][leftX] == 3 ||
      g_matches[matchNum].map[bottomY][rightX] == 3) {
    return true;
  }
  if (g_matches[matchNum].map[gridY][gridX] == 3) {
    return true;
  }
  return false;
}

bool IsNextColliding(int matchNum, int x, int y) {
  int leftX = (x - PLAYER_SIZE / 2) / GRID;
  int rightX = (x + PLAYER_SIZE / 2 - 1) / GRID;
  int topY = (y - PLAYER_SIZE / 2) / GRID;
  int bottomY = (y + PLAYER_SIZE / 2 - 1) / GRID;

  if (g_matches[matchNum].map[topY][leftX] == 6 ||
      g_matches[matchNum].map[topY][rightX] == 6) {
    return true;
  }
  return false;
}

void movePlayer(int matchNum) {
  int newX = g_matches[matchNum].player1.x + g_matches[matchNum].player1.dx;
  int newY = g_matches[matchNum].player1.y + g_matches[matchNum].player1.dy;
  printf("%d %d, %d %d\n", g_matches[matchNum].player1.x,
         g_matches[matchNum].player1.dx, g_matches[matchNum].player1.y,
         g_matches[matchNum].player1.dy);
  bool isVerticalCollision =
      IsColliding(matchNum, g_matches[matchNum].player1.x, newY);
  bool isHorizontalCollision =
      IsColliding(matchNum, newX, g_matches[matchNum].player1.y);
  bool isSlopeGoRightCollision = IsSlopeGoRightColliding(
      matchNum, g_matches[matchNum].player1.x, g_matches[matchNum].player1.y);
  bool isSlopeGoLeftCollision = IsSlopeGoLeftColliding(
      matchNum, g_matches[matchNum].player1.x, g_matches[matchNum].player1.y);

  // ���� �浹 ó��
  if (!isVerticalCollision) {
    g_matches[matchNum].player1.y = newY;
    if (!g_matches[matchNum].player1.EnhancedJumpPower) {
      g_matches[matchNum].player1.isJumping = true;
    }
  } else {
    // �ٴ� �浹 �� y�� ��ġ ����
    if (g_matches[matchNum].player1.dy > 0) {
      if (!IsColliding(matchNum, g_matches[matchNum].player1.x,
                       g_matches[matchNum].player1.y + 1)) {
        g_matches[matchNum].player1.y += 1;
      }
    }
    g_matches[matchNum].player1.dy = 0;  // �浹 �� y�� �ӵ� �ʱ�ȭ
    g_matches[matchNum].player1.isJumping = false;
    g_matches[matchNum].player1.isSliding = false;
  }

  // ���� �浹 ó��
  if (!isHorizontalCollision) {
    g_matches[matchNum].player1.x = newX;
  } else {
    g_matches[matchNum].player1.dx = 0;  // �浹 �� x�� �ӵ� �ʱ�ȭ
  }

  if (isSlopeGoRightCollision) {
    g_matches[matchNum].player1.isSliding = true;

    g_matches[matchNum].player1.dy = 1;  // ���� ������ �̲����� �ӵ�
    g_matches[matchNum].player1.dx = 3;  // ������ �Ʒ��� �̲�����
    newX = g_matches[matchNum].player1.x + g_matches[matchNum].player1.dx;
    newY = g_matches[matchNum].player1.y + g_matches[matchNum].player1.dy;
    g_matches[matchNum].player1.x = newX;
    g_matches[matchNum].player1.y = newY;
  }

  if (isSlopeGoLeftCollision) {
    g_matches[matchNum].player1.isSliding = true;

    g_matches[matchNum].player1.dy = 1;  // ���� ������ �̲����� �ӵ�
    g_matches[matchNum].player1.dx = -3;  // ������ �Ʒ��� �̲�����
    newX = g_matches[matchNum].player1.x + g_matches[matchNum].player1.dx;
    newY = g_matches[matchNum].player1.y + g_matches[matchNum].player1.dy;
    g_matches[matchNum].player1.x = newX;
    g_matches[matchNum].player1.y = newY;
  }
}

void moveBullets(int matchNum) {
  for (auto it = g_matches[matchNum].g_bullets.begin();
       it != g_matches[matchNum].g_bullets.end();) {
    it->x += it->dx;
    it->y += it->dy;
    if (it->x < 0 || it->x > BOARD_WIDTH) {
      it = g_matches[matchNum].g_bullets.erase(it);
    } else {
      ++it;
    }
  }
}

void CheckCollisions(int matchNum) {
  CheckEnemyPlayerCollisions(matchNum);
  CheckItemPlayerCollisions(matchNum);
  CheckPlayerBulletCollisions(matchNum);
  CheckPlayersCollisions(matchNum);
}

void CheckEnemyPlayerCollisions(int matchNum) {
  for (auto it = g_matches[matchNum].g_enemies.begin();
       it != g_matches[matchNum].g_enemies.end();) {
    if (g_matches[matchNum].player1.x >= it->x * GRID &&
        g_matches[matchNum].player1.x <= (it->x + 1) * GRID &&
        g_matches[matchNum].player1.y >= it->y * GRID &&
        g_matches[matchNum].player1.y <= (it->y + 1) * GRID) {
      g_matches[matchNum].player1.dx = 4;
      g_matches[matchNum].player1.isCharging = false;
      g_matches[matchNum].player1.jumpSpeed = 0;
      ++it;  // �浹 �� �ݺ��ڸ� ������ŵ�ϴ�.
    } else {
      ++it;  // �浹�� �߻����� �ʾ��� ���� �ݺ��ڸ� ������ŵ�ϴ�.
    }
  }
}

void CheckItemPlayerCollisions(int matchNum) {
  for (auto it = g_matches[matchNum].g_items.begin();
       it != g_matches[matchNum].g_items.end();) {
    if (g_matches[matchNum].player1.x >= it->x * GRID &&
        g_matches[matchNum].player1.x <= (it->x + 1) * GRID &&
        g_matches[matchNum].player1.y >= it->y * GRID &&
        g_matches[matchNum].player1.y <= (it->y + 1) * GRID) {
      g_matches[matchNum].player1.EnhancedJumpPower = true;
      g_matches[matchNum].player1.isJumping = false;
      it->disable = true;
      it->interval = 60;
    }
    ++it;
  }
}

void CheckPlayerBulletCollisions(int matchNum) {
  for (auto it = g_matches[matchNum].g_bullets.begin();
       it != g_matches[matchNum].g_bullets.end();) {
    if (it->x >= g_matches[matchNum].player1.x - PLAYER_SIZE &&
        it->x <= g_matches[matchNum].player1.x + PLAYER_SIZE &&
        it->y >= g_matches[matchNum].player1.y - PLAYER_SIZE &&
        it->y <= g_matches[matchNum].player1.y + PLAYER_SIZE) {
      // �÷��̾ �ڷ� ��ħ
      g_matches[matchNum].player1.dx = it->dx * 2;
      g_matches[matchNum].player1.isCharging = false;
      g_matches[matchNum].player1.jumpSpeed = 0;
      g_matches[matchNum].player1.damaged = true;
      // �÷��̾�� �浹 �� ����
      it = g_matches[matchNum].g_bullets.erase(it);
    } else {
      ++it;
    }
  }
}

void CheckPlayersCollisions(int matchNum) {}

void updateSendParam(int matchNum) {
  // player 1
  g_matches[matchNum].SPlayer1.x = g_matches[matchNum].player1.x;
  g_matches[matchNum].SPlayer1.y = g_matches[matchNum].player1.y;
  g_matches[matchNum].SPlayer1.acting = 0;  // ���� �浹ó�� ���� �߰�
  // player 2
  g_matches[matchNum].SPlayer2.x = g_matches[matchNum].player2.x;
  g_matches[matchNum].SPlayer2.y = g_matches[matchNum].player2.y;
  g_matches[matchNum].SPlayer2.acting = 0;  // ���� �浹ó�� ���� �߰�
}
