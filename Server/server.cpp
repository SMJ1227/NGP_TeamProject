#define SERVERPORT 9000
#define BUFSIZE 512

#include "common.h"
#include <windows.h>  // windows 관련 함수 포함

#include <iostream>
#include <vector>
#include <print>
#include <thread>

#include "map.h"
#include "sendParam.hpp"

CRITICAL_SECTION cs;
HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
std::vector<DWORD> deleteThreadIDs;

typedef struct Player {
  int x, y;
  int dx = 0, dy = 0;
  int jumpSpeed = 0;
  bool isCharging = false;
  bool isJumping = false;
  bool isSliding = false;
  bool face = 0;  // face: left, right
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
};

struct recvParam {
  SOCKET client_sock = NULL;
  char matchNum = NULL;
  char playerNum = NULL;
};

typedef struct MATCH {
  SOCKET client_sock[2]{NULL, NULL};
  HANDLE recvThread[2]{NULL, NULL};
  HANDLE logicThread;
  HANDLE hEvent;
  Player player1;
  DWORD recvThreadID[2];
  DWORD logicThreadID;
  sendParam::playerInfo SPlayer1;
  Player player2;
  sendParam::playerInfo SPlayer2;
  int map[MAP_HEIGHT][MAP_WIDTH];
  char matchNum = 0;
  char p1 = 'a';
  char p2 = 'a';
  int mapNum = 1;
  int score = 0;
  int shootInterval = 0;
  bool header = false;
  std::vector<Item> g_items;
  std::vector<Enemy> g_enemies;
  std::vector<Bullet> g_bullets;

  bool operator==(const MATCH& other) {
    return client_sock[0] == other.client_sock[0] &&
           client_sock[1] == other.client_sock[1];
  }
};
std::vector<MATCH> g_matches;

// 초기화 함수
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
// 이동 함수
void updatePlayerD(int matchNum);
void applyGravity(int matchNum);
bool IsColliding(int matchNum, int x, int y);
bool IsSlopeGoRightColliding(int matchNum, int x, int y);
bool IsSlopeGoLeftColliding(int matchNum, int x, int y);
int IsNextColliding(int matchNum);
void movePlayer(int matchNum);
void moveBullets(int matchNum);
// 오브젝트 충돌처리
void CheckCollisions(int matchNum);
void CheckEnemyPlayerCollisions(int matchNum);
void CheckItemPlayerCollisions(int matchNum);
void CheckPlayerBulletCollisions(int matchNum);
void CheckPlayersCollisions(int matchNum);
void updateSendParam(int matchNum);

// 매치를 삭제하는 함수
void closeSocketFunc(SOCKET client_sock, char matchNum, char playerNum) {
  printf("%dmatch %dplayer disconnectd\n", matchNum, playerNum);

  // deleteThreadIDs에 삭제해야할 스레드 ID 추가, 이 함수 호출한 recv스레드는 호출 후 자살해서 벡터에 추가X
  if (playerNum == 0) deleteThreadIDs.push_back(g_matches[matchNum].recvThreadID[1]);
  else if (playerNum == 1) deleteThreadIDs.push_back(g_matches[matchNum].recvThreadID[0]);
  deleteThreadIDs.push_back(g_matches[matchNum].logicThreadID);
  for (const auto& threadID : deleteThreadIDs)
  {
      printf("deleteThreadIDs: %d\n", threadID);
  }
  SetEvent(g_matches[matchNum].hEvent);
  std::vector<MATCH>::iterator iter = g_matches.begin();
  for (int i = 0; i < matchNum; i++) iter++;
  g_matches.erase(std::remove(g_matches.begin(), g_matches.end(), *iter),
                  g_matches.end());
  // 디버그용 출력
  printf("delete match: %d\n", matchNum);

  for (int i = 0; i < g_matches.size(); i++) {
    g_matches[i].matchNum = i;
  }

  
}

void UpdateGameLogic(int matchNum) {
  updatePlayerD(matchNum);
  applyGravity(matchNum);
  movePlayer(matchNum);
  if (int isNext =
          IsNextColliding(matchNum)) {  // 1(p1), 2(p2)를 리턴하면 조건문 진입
    // p1이 이기면 score+=10, p2가 이기면 score+=1
    if (isNext == 1)
      g_matches[matchNum].score += 10;
    else if (isNext == 2)
      g_matches[matchNum].score += 1;
    // 추후 수정
    if (g_matches[matchNum].mapNum == 1) {
      g_matches[matchNum].mapNum = 2;
      InitMap(matchNum, map1);
    } else if (g_matches[matchNum].mapNum == 2) {
      g_matches[matchNum].mapNum = 3;
      InitMap(matchNum, map2);
    }

    // mapNum 3 -> 4 게임종료, ### mapNum == 4이면 게임 종료로 판단?
    // 비정상 종료를 몰수승으로 판단하면 게임 종료 시에는 반드시 map_num = 4인
    // 상태로 게임 종료
    // -> CHANGE_MAP 패킷의 mapNum으로 게임 종료, 승 패를 알림
    // 클라이언트에 보낼때는 5와 6을 나눠서 보냄, 5는승리, 6은 패배
    // 서버 입장에서 5는 p1승, 6은 p2승
    else if (g_matches[matchNum].mapNum == 3 &&
             g_matches[matchNum].score >= 20) {
      g_matches[matchNum].mapNum = 5;
    } else if (g_matches[matchNum].mapNum == 3 &&
               g_matches[matchNum].score < 20) {
      g_matches[matchNum].mapNum = 6;
    }
    // printf("이동한 맵 넘버: %d\n", g_matches[matchNum].mapNum);

    DeleteAllEnemies(matchNum);
    DeleteAllBullets(matchNum);
    DeleteAllItems(matchNum);
    initPlayer(matchNum);
    initEnemy(matchNum);
    initItem(matchNum);

    g_matches[matchNum].header = true;

  } 
  else {
    moveBullets(matchNum);
    g_matches[matchNum].shootInterval++;
    if (g_matches[matchNum].shootInterval > 120) {
      ShootBullet(matchNum);
      g_matches[matchNum].shootInterval = 0;
    }
    for (auto& item : g_matches[matchNum].g_items) {
      if (item.interval <= 0) {
        item.disable = false;
      } else {
        item.interval--;
      }
    }
    CheckCollisions(matchNum);
    updateSendParam(matchNum);
  }
}

DWORD WINAPI GameLogicUpdateThread(LPVOID lpParam) {
  int matchNum = *(int*)lpParam;
  delete (int*)lpParam;

  while (true) {
    // recv 스레드에서 데이터 처리 준비 완료 신호 대기
    WaitForSingleObject(g_matches[matchNum].hEvent, INFINITE);

    EnterCriticalSection(&cs);
    std::vector<DWORD>::iterator iter = std::find(deleteThreadIDs.begin(), deleteThreadIDs.end(), GetCurrentThreadId());
    if (iter != deleteThreadIDs.end()){
        deleteThreadIDs.erase(iter);
        printf("LogicThread %d is deleted\n", GetCurrentThreadId());
        LeaveCriticalSection(&cs);
        ExitThread(0);
    }
    LeaveCriticalSection(&cs);

    while (!(matchNum < 0) && matchNum < g_matches.size() &&
           g_matches[matchNum].matchNum != matchNum) {
      // 디버그용 출력
      printf("matchNum--\n");
      matchNum--;
    }

    EnterCriticalSection(&cs);
    UpdateGameLogic(matchNum);
    LeaveCriticalSection(&cs);
    // send 부분
    char sendBuf[BUFSIZE];
    int sendSize;

    for (int i = 0; i < 2; ++i) {
      if (g_matches[matchNum].client_sock[i] == NULL) {
        // printf("클라이언트 %d 소켓이 NULL입니다.\n", i);
        continue;
      }
      if (!g_matches[matchNum].header) {  // playerinfo
        sendParam::sendParam sendParam;
        sendSize = sizeof(sendParam::sendParam);
        if (i == 0) {
          sendParam.myInfo = g_matches[matchNum].SPlayer1;
          sendParam.otherInfo = g_matches[matchNum].SPlayer2;

        } else if (i == 1) {
          sendParam.myInfo = g_matches[matchNum].SPlayer2;
          sendParam.otherInfo = g_matches[matchNum].SPlayer1;
        }
        memcpy(sendBuf, &sendParam, sizeof(sendParam));
        // g_bullets 데이터 추가 직렬화
        size_t offset = sizeof(sendParam::sendParam);  // sendParam 크기
        size_t bulletDataSize = g_matches[matchNum].g_bullets.size() *
                                sizeof(sendParam::Bullet);  // 원래 불렛 크기
        sendSize += bulletDataSize;
        if (!g_matches[matchNum].g_bullets.empty()) {
          memcpy(sendBuf + offset, g_matches[matchNum].g_bullets.data(),
                 bulletDataSize);  // 불렛들을 바로 보냄
        }
        int retval =
            send(g_matches[matchNum].client_sock[i], sendBuf, sendSize, 0);
        if (retval == SOCKET_ERROR) {
          // printf("client %d fail: %d\n", i, WSAGetLastError());
        } else {
          // printf("client %d send: %d byte\n", i, retval);
        }
      } 
      else {
        sendParam::MapInfoPacket mapInfoPacket;
        sendSize = sizeof(sendParam::MapInfoPacket);
        // p1 승일때
        if (g_matches[matchNum].mapNum == 5) {
          if (i == 0)
            mapInfoPacket.info.mapNum = 5;
          else if (i == 1)
            mapInfoPacket.info.mapNum = 6;
        }  // 수정
        else if (g_matches[matchNum].mapNum == 6) {
          if (i == 0)
            mapInfoPacket.info.mapNum = 6;
          else if (i == 1)
            mapInfoPacket.info.mapNum = 5;
        } else
          mapInfoPacket.info.mapNum = g_matches[matchNum].mapNum;
        memcpy(sendBuf, &mapInfoPacket, sendSize);
        int retval =
            send(g_matches[matchNum].client_sock[i], sendBuf, sendSize, 0);
        if (retval == SOCKET_ERROR) {
          // printf("clien %d fail: %d\n", i, WSAGetLastError());
        } else {
          // printf("client %d send %d bute\n", i, retval);
        }
      }
    }
    g_matches[matchNum].header = false;
    // 이벤트 해제
    ResetEvent(g_matches[matchNum].hEvent);
  }
  CloseHandle(g_matches[matchNum].hEvent);
  return 0;
}

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

  printf("\n[TCP server] client connect: IP address=%s, port number=%d\n", addr, ntohs(clientaddr.sin_port));

  while (1) {
      EnterCriticalSection(&cs);
      std::vector<DWORD>::iterator iter = std::find(
          deleteThreadIDs.begin(), deleteThreadIDs.end(), GetCurrentThreadId());
      if (iter != deleteThreadIDs.end())
      {
          printf("RecvThread %d is deleted\n", GetCurrentThreadId());
          deleteThreadIDs.erase(iter);
          shutdown(client_sock, SD_BOTH);
          closesocket(client_sock);
          LeaveCriticalSection(&cs);
          ExitThread(0);
      }
      LeaveCriticalSection(&cs);
    // 벡터의 유효한 범위 내에서, 현재 매치의 번호와, 매치[현재 매치번호]의 매치
    // 번호가 일치하는지 비교한다, 다르다면 감소
    while (matchNum >= 0 && matchNum < g_matches.size() &&
           g_matches[matchNum].matchNum != matchNum) {
      // 디버그옹 출력
      printf("matchNum--\n");
      matchNum--;
    }
    retval = recv(client_sock, buf, BUFSIZE, 0);
    if (retval == SOCKET_ERROR) {
      err_display("recv()");
      /*closeSocketFunc(client_sock, matchNum, playerNum);
      ExitThread(0);*/
      break;
    } else if (retval == 0) {
      
      break;
    }

    // 받은 데이터 출력
    buf[retval] = '\0';
    char input0 = buf[0];
    char input1 = buf[1];
    std::println("{:d}match player{:d} recv data: {:?}, {:?}", matchNum,
                 playerNum, buf[0], buf[1]);
    if (playerNum == 0) {
      if (buf[1] == '\0') { // 하나의 값만 들어옴
        if (!g_matches[matchNum].player1.isCharging) {
          g_matches[matchNum].p1 = input0;
        } 
        else {
          if (input0 == ' ') {
            g_matches[matchNum].p1 = input0;
          } 
          else if (input0 == '\b') {
            g_matches[matchNum].p1 = input0;
          }
        }
      } 
      else {
        g_matches[matchNum].p1 = input1;
      }
    }
    else if (playerNum == 1) {
      if (buf[1] == '\0') {  // 하나의 값만 들어옴
        if (!g_matches[matchNum].player2.isCharging) {
          g_matches[matchNum].p2 = input0;
        } else {
          if (input0 == ' ') {
            g_matches[matchNum].p2 = input0;
          } else if (input0 == '\b') {
            g_matches[matchNum].p2 = input0;
          }
        }
      } else {
        g_matches[matchNum].p2 = input1;
      }
    }
    // 이벤트 해제
    SetEvent(g_matches[matchNum].hEvent);
  }
  // 소켓 닫기
  closesocket(client_sock);
  printf("[TCP server] client quit: IP address=%s, port number=%d\n", addr,
         ntohs(clientaddr.sin_port));
  closeSocketFunc(client_sock, matchNum, playerNum);
  // closeSocketFunc으로 매치가 삭제돼서 SetEvent를 호출하면 다른 매치의 이벤트가 삭제됨 아닌가 로직은 삭제되는데 리시브가 삭제안됨
  //SetEvent(g_matches[matchNum].hEvent);
  ExitThread(0);
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

  // 데이터 통신에 사용할 변수
  struct sockaddr_in clientaddr;
  int addrlen;
  HANDLE hThread;
  recvParam* rParam;
  int* matchNumParam;
  InitializeCriticalSection(&cs);

  while (1) {
    printf("waiting for client...\n");
    addrlen = sizeof(clientaddr);
    rParam = new recvParam{};
    matchNumParam = new int{};
    rParam->client_sock =
        accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
    if (rParam->client_sock == INVALID_SOCKET) {
      err_display("accept()");
      break;
    }
    // 매치 생성 조건 - 현재 매치가 없거나(0), 마지막 매치의 player가 다 차있으면 생성 
    // 플레이어 1 생성 조건: 마지막 매치의 소켓0번이 비었으면 생성
    // 플레이어 2 생성 조건: 마지막 매치의 소켓1이 차있고 소켓2가 비었으면 생성
    // 타이머 생성 조건: 플레이어 1 생성할 때
    char addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
    /*printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n", addr,
           ntohs(clientaddr.sin_port));*/
    // 매치 생성
    if (g_matches.size() == 0 ||
        (!g_matches.empty() && g_matches.back().client_sock[0] != NULL &&
         g_matches.back().client_sock[1] != NULL)) {
      //g_matches.push_back(MATCH());
      MATCH newMatch;
      newMatch.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);  // 이벤트 생성
      g_matches.push_back(newMatch);
    }
    // 플레이어1 스레드, 타이머 스레드 생성
    if (g_matches.back().client_sock[0] == NULL) {
      rParam->playerNum = 0;
      rParam->matchNum = g_matches.size() - 1;
      *matchNumParam = g_matches.size() - 1;
      // g_matches의 클라이언트 소켓, 매치 넘버 업데이트
      g_matches.back().client_sock[0] = rParam->client_sock;
      g_matches.back().matchNum = g_matches.size() - 1;
      initAll(*matchNumParam);
      // 수신 스레드 생성
      g_matches.back().recvThread[0] =
          CreateThread(NULL, 0, RecvProcessClient, rParam, 0,
                       &g_matches.back().recvThreadID[0]);
      // 디버그용 출력
      printf("%zu match is waiting.. num of client: %d\n", g_matches.size() - 1,
             1);
      // 타이머 스레드 생성
      g_matches.back().logicThread =
          CreateThread(NULL, 0, GameLogicUpdateThread, matchNumParam, 0, &g_matches.back().logicThreadID);
    }
    // 플레이어2 스레드 생성

    else if (g_matches.back().client_sock[0] != NULL &&
             g_matches.back().client_sock[1] == NULL) {
      printf("enter for 2\n");
      rParam->playerNum = 1;
      rParam->matchNum = g_matches.size() - 1;
      g_matches.back().client_sock[1] = rParam->client_sock;
      // 디버그용 출력
      printf("%dmatch %dplayer thread created\n", rParam->matchNum,
             rParam->playerNum);
      g_matches.back().recvThread[1] =
          CreateThread(NULL, 0, RecvProcessClient, rParam, 0,
                       &g_matches.back().recvThreadID[1]);
      initPlayer(*matchNumParam);
    }
  }

  // 소켓 닫기
  closesocket(listen_sock);
  DeleteCriticalSection(&cs);
  // 윈속 종료
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
      if (g_matches[matchNum].map[y][x] == 4) {  // 적
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
    newBullet.x = (enemy.x + 1) * GRID;  // 적의 위치에서 총알이 나가도록 설정
    newBullet.y = enemy.y * GRID + GRID / 2;
    g_matches[matchNum].g_bullets.push_back(newBullet);
  }
}

void DeleteAllBullets(int matchNum) { g_matches[matchNum].g_bullets.clear(); }

void initAll(int matchNum) {
  InitMap(matchNum, map0);
  initPlayer(matchNum);
  initItem(matchNum);
  initEnemy(matchNum);
}

void updatePlayerD(int matchNum) {
  // player1 처리
  if (g_matches[matchNum].p1 == '0') {
    if (g_matches[matchNum].player1.dx >= -3) {
      g_matches[matchNum].player1.dx -= 1;
      g_matches[matchNum].player1.face = false;
    }
  } 
  else if (g_matches[matchNum].p1 == '1') {
    if (g_matches[matchNum].player1.dx <= 3) {
      g_matches[matchNum].player1.dx += 1;
      g_matches[matchNum].player1.face = true;
    }
  } 
  else if (g_matches[matchNum].p1 == 'a') {
    // 왼쪽, 오른쪽 키가 모두 눌리지 않은 상태
    if (g_matches[matchNum].player1.dx > 0) {
      g_matches[matchNum].player1.dx -= 1;
    } else if (g_matches[matchNum].player1.dx < 0) {
      g_matches[matchNum].player1.dx += 1;
    }
  } 
  else if (g_matches[matchNum].p1 == ' ') {  // 점프 차징
    g_matches[matchNum].player1.spaceKeyReleased = false;
    if (!g_matches[matchNum].player1.isJumping &&
        g_matches[matchNum].player1.jumpSpeed > -20) {
      g_matches[matchNum].player1.isCharging = true;
      g_matches[matchNum].player1.dx = 0;
      g_matches[matchNum].player1.jumpSpeed -= 1;
      if (g_matches[matchNum].player1.EnhancedJumpPower == 1) {
        g_matches[matchNum].player1.jumpSpeed = -20;
      }
    }
  } 
  else if (g_matches[matchNum].p1 == '\b') {  // 점프 뛰기
    if (!g_matches[matchNum].player1.spaceKeyReleased &&
        g_matches[matchNum].player1.isCharging) {
      g_matches[matchNum].player1.dy = g_matches[matchNum].player1.jumpSpeed;
      g_matches[matchNum].player1.jumpSpeed = 0;
      g_matches[matchNum].player1.isCharging = false;
      g_matches[matchNum].player1.isJumping = true;
      if (g_matches[matchNum].player1.EnhancedJumpPower == 1) {
        g_matches[matchNum].player1.EnhancedJumpPower = 0;
      }
      g_matches[matchNum].player1.spaceKeyReleased = true;
    }
  } 
  else if (g_matches[matchNum].p1 == 'a') {
    // 왼쪽, 오른쪽 키가 모두 눌리지 않은 상태
    if (g_matches[matchNum].player1.dx > 0) {
      g_matches[matchNum].player1.dx -= 1;
    } else if (g_matches[matchNum].player1.dx < 0) {
      g_matches[matchNum].player1.dx += 1;
    }
  } 
  

  // player2 처리
  if (g_matches[matchNum].p2 == '0') {
    if (g_matches[matchNum].player2.dx >= -3) {
      g_matches[matchNum].player2.dx -= 1;
      g_matches[matchNum].player2.face = false;
    }
  } else if (g_matches[matchNum].p2 == '1') {
    if (g_matches[matchNum].player2.dx <= 3) {
      g_matches[matchNum].player2.dx += 1;
      g_matches[matchNum].player2.face = true;
    }
  } else if (g_matches[matchNum].p2 == 'a') {
    // 왼쪽, 오른쪽 키가 모두 눌리지 않은 상태
    if (g_matches[matchNum].player2.dx > 0) {
      g_matches[matchNum].player2.dx -= 1;
    } else if (g_matches[matchNum].player2.dx < 0) {
      g_matches[matchNum].player2.dx += 1;
    }
  } else if (g_matches[matchNum].p2 == ' ') {  // 점프 차징
    g_matches[matchNum].player2.spaceKeyReleased = false;
    if (!g_matches[matchNum].player2.isJumping &&
        g_matches[matchNum].player2.jumpSpeed > -20) {
      g_matches[matchNum].player2.isCharging = true;
      g_matches[matchNum].player2.dx = 0;
      g_matches[matchNum].player2.jumpSpeed -= 1;
      if (g_matches[matchNum].player2.EnhancedJumpPower == 1) {
        g_matches[matchNum].player2.jumpSpeed = -20;
      }
    }
  } else if (g_matches[matchNum].p2 == '\b') {  // 점프 뛰기
    if (!g_matches[matchNum].player2.spaceKeyReleased &&
        g_matches[matchNum].player2.isCharging) {
      g_matches[matchNum].player2.dy = g_matches[matchNum].player2.jumpSpeed;
      g_matches[matchNum].player2.jumpSpeed = 0;
      g_matches[matchNum].player2.isCharging = false;
      g_matches[matchNum].player2.isJumping = true;
      if (g_matches[matchNum].player2.EnhancedJumpPower == 1) {
        g_matches[matchNum].player2.EnhancedJumpPower = 0;
      }
      g_matches[matchNum].player2.spaceKeyReleased = true;
    }
  } else if (g_matches[matchNum].p2 == 'a') {
    // 왼쪽, 오른쪽 키가 모두 눌리지 않은 상태
    if (g_matches[matchNum].player2.dx > 0) {
      g_matches[matchNum].player2.dx -= 1;
    } else if (g_matches[matchNum].player2.dx < 0) {
      g_matches[matchNum].player2.dx += 1;
    }
  }
}

void applyGravity(int matchNum) {
  if (g_matches[matchNum].player1.dy < 20) {
    g_matches[matchNum].player1.dy += GRAVITY;
  }
  if (g_matches[matchNum].player2.dy < 20) {
    g_matches[matchNum].player2.dy += GRAVITY;
  }
}

bool IsColliding(int matchNum, int x, int y) {
  int gridX = x / GRID;
  int gridY = y / GRID;

  if (gridX < 0 || gridX >= MAP_WIDTH || gridY < 0 || gridY >= MAP_HEIGHT) {
    return true;
  }

  if (g_matches[matchNum].map[gridY][gridX] == 0 ||
      g_matches[matchNum].map[gridY][gridX] == 2 ||
      g_matches[matchNum].map[gridY][gridX] == 3)
  {
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

  // 충돌 감지
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

  // 충돌 감지
  if (g_matches[matchNum].map[bottomY][leftX] == 3 ||
      g_matches[matchNum].map[bottomY][rightX] == 3) {
    return true;
  }
  if (g_matches[matchNum].map[gridY][gridX] == 3) {
    return true;
  }
  return false;
}

// 정수타입 리턴
// 0: 충돌하지 않음, 1: p1과 충돌, 2: p2와 충돌
int IsNextColliding(int matchNum) {  // p1인지 p2인지 알기 위해 bool보단 리턴타입을 정수로 리턴하도록 바꾸기
  int leftX = (g_matches[matchNum].player1.x - PLAYER_SIZE / 2) / GRID;
  int rightX = (g_matches[matchNum].player1.x + PLAYER_SIZE / 2 - 1) / GRID;
  int topY = (g_matches[matchNum].player1.y - PLAYER_SIZE / 2) / GRID;
  int bottomY = (g_matches[matchNum].player1.y + PLAYER_SIZE / 2 - 1) / GRID;

  if (g_matches[matchNum].map[topY][leftX] == 6 ||
      g_matches[matchNum].map[topY][rightX] == 6) {
    return 1;
  }

  leftX = (g_matches[matchNum].player2.x - PLAYER_SIZE / 2) / GRID;
  rightX = (g_matches[matchNum].player2.x + PLAYER_SIZE / 2 - 1) / GRID;
  topY = (g_matches[matchNum].player2.y - PLAYER_SIZE / 2) / GRID;
  bottomY = (g_matches[matchNum].player2.y + PLAYER_SIZE / 2 - 1) / GRID;

  if (g_matches[matchNum].map[topY][leftX] == 6 ||
      g_matches[matchNum].map[topY][rightX] == 6) {
    return 2;
  }

  return 0;
}

void movePlayer(int matchNum) {
    // player 1
  int newX = g_matches[matchNum].player1.x + g_matches[matchNum].player1.dx;
  int newY = g_matches[matchNum].player1.y + g_matches[matchNum].player1.dy;

  bool isVerticalCollision =
      IsColliding(matchNum, g_matches[matchNum].player1.x, newY);
  bool isHorizontalCollision =
      IsColliding(matchNum, newX, g_matches[matchNum].player1.y);
  bool isSlopeGoRightCollision = IsSlopeGoRightColliding(
      matchNum, g_matches[matchNum].player1.x, g_matches[matchNum].player1.y);
  bool isSlopeGoLeftCollision = IsSlopeGoLeftColliding(
      matchNum, g_matches[matchNum].player1.x, g_matches[matchNum].player1.y);

  // 수직 충돌 처리
  if (!isVerticalCollision) {
    g_matches[matchNum].player1.y = newY;
    if (!g_matches[matchNum].player1.EnhancedJumpPower) {
      g_matches[matchNum].player1.isJumping = true;
    }
  } else {
    // 바닥 충돌 시 y축 위치 보정
    if (g_matches[matchNum].player1.dy > 0) {
      if (!IsColliding(matchNum, g_matches[matchNum].player1.x,
                       g_matches[matchNum].player1.y + 1)) {
        g_matches[matchNum].player1.y += 1;
      }
    }
    g_matches[matchNum].player1.dy = 0;  // 충돌 후 y축 속도 초기화
    g_matches[matchNum].player1.isJumping = false;
    g_matches[matchNum].player1.isSliding = false;
    g_matches[matchNum].player1.EnhancedJumpPower = false;
  }

  // 수평 충돌 처리
  if (!isHorizontalCollision) {
    g_matches[matchNum].player1.x = newX;
  } else {
    g_matches[matchNum].player1.dx = 0;  // 충돌 후 x축 속도 초기화
  }

  if (isSlopeGoRightCollision) {
    g_matches[matchNum].player1.isSliding = true;

    g_matches[matchNum].player1.dy = 1;  // 경사면 위에서 미끄러짐 속도
    g_matches[matchNum].player1.dx = 3;  // 오른쪽 아래로 미끄러짐
    newX = g_matches[matchNum].player1.x + g_matches[matchNum].player1.dx;
    newY = g_matches[matchNum].player1.y + g_matches[matchNum].player1.dy;
    g_matches[matchNum].player1.x = newX;
    g_matches[matchNum].player1.y = newY;
  }

  if (isSlopeGoLeftCollision) {
    g_matches[matchNum].player1.isSliding = true;

    g_matches[matchNum].player1.dy = 1;  // 경사면 위에서 미끄러짐 속도
    g_matches[matchNum].player1.dx = -3;  // 오른쪽 아래로 미끄러짐
    newX = g_matches[matchNum].player1.x + g_matches[matchNum].player1.dx;
    newY = g_matches[matchNum].player1.y + g_matches[matchNum].player1.dy;
    g_matches[matchNum].player1.x = newX;
    g_matches[matchNum].player1.y = newY;
  }

  // player 2
  newX = g_matches[matchNum].player2.x + g_matches[matchNum].player2.dx;
  newY = g_matches[matchNum].player2.y + g_matches[matchNum].player2.dy;

  isVerticalCollision =
      IsColliding(matchNum, g_matches[matchNum].player2.x, newY);
  isHorizontalCollision =
      IsColliding(matchNum, newX, g_matches[matchNum].player2.y);
  isSlopeGoRightCollision = IsSlopeGoRightColliding(
      matchNum, g_matches[matchNum].player2.x, g_matches[matchNum].player2.y);
  isSlopeGoLeftCollision = IsSlopeGoLeftColliding(
      matchNum, g_matches[matchNum].player2.x, g_matches[matchNum].player2.y);

  // 수직 충돌 처리
  if (!isVerticalCollision) {
    g_matches[matchNum].player2.y = newY;
    if (!g_matches[matchNum].player2.EnhancedJumpPower) {
      g_matches[matchNum].player2.isJumping = true;
    }
  } else {
    // 바닥 충돌 시 y축 위치 보정
    if (g_matches[matchNum].player2.dy > 0) {
      if (!IsColliding(matchNum, g_matches[matchNum].player2.x,
                       g_matches[matchNum].player2.y + 1)) {
        g_matches[matchNum].player2.y += 1;
      }
    }
    g_matches[matchNum].player2.dy = 0;  // 충돌 후 y축 속도 초기화
    g_matches[matchNum].player2.isJumping = false;
    g_matches[matchNum].player2.isSliding = false;
    g_matches[matchNum].player2.EnhancedJumpPower = false;
  }

  // 수평 충돌 처리
  if (!isHorizontalCollision) {
    g_matches[matchNum].player2.x = newX;
  } else {
    g_matches[matchNum].player2.dx = 0;  // 충돌 후 x축 속도 초기화
  }

  if (isSlopeGoRightCollision) {
    g_matches[matchNum].player2.isSliding = true;

    g_matches[matchNum].player2.dy = 1;  // 경사면 위에서 미끄러짐 속도
    g_matches[matchNum].player2.dx = 3;  // 오른쪽 아래로 미끄러짐
    newX = g_matches[matchNum].player2.x + g_matches[matchNum].player2.dx;
    newY = g_matches[matchNum].player2.y + g_matches[matchNum].player2.dy;
    g_matches[matchNum].player2.x = newX;
    g_matches[matchNum].player2.y = newY;
  }

  if (isSlopeGoLeftCollision) {
    g_matches[matchNum].player2.isSliding = true;

    g_matches[matchNum].player2.dy = 1;  // 경사면 위에서 미끄러짐 속도
    g_matches[matchNum].player2.dx = -3;  // 오른쪽 아래로 미끄러짐
    newX = g_matches[matchNum].player2.x + g_matches[matchNum].player2.dx;
    newY = g_matches[matchNum].player2.y + g_matches[matchNum].player2.dy;
    g_matches[matchNum].player2.x = newX;
    g_matches[matchNum].player2.y = newY;
  }
}

void moveBullets(int matchNum) {
  for (auto it = g_matches[matchNum].g_bullets.begin();
       it != g_matches[matchNum].g_bullets.end();) {
    it->x += 2;
    if (it->x < 0 || it->x > BOARD_WIDTH) {
      it = g_matches[matchNum].g_bullets.erase(it);
    } else {
      ++it;
    }
  }
}

void CheckCollisions(int matchNum) {
  CheckEnemyPlayerCollisions(matchNum);
    if (g_matches[matchNum].client_sock[1] != NULL)
    {
        CheckItemPlayerCollisions(matchNum);
    }
  CheckPlayerBulletCollisions(matchNum);
  CheckPlayersCollisions(matchNum);
}

void CheckEnemyPlayerCollisions(int matchNum) {
  //player 1
  for (auto it = g_matches[matchNum].g_enemies.begin();
       it != g_matches[matchNum].g_enemies.end();) {
    if (g_matches[matchNum].player1.x >= it->x * GRID &&
        g_matches[matchNum].player1.x <= (it->x + 1) * GRID &&
        g_matches[matchNum].player1.y >= it->y * GRID &&
        g_matches[matchNum].player1.y <= (it->y + 1) * GRID) {
      g_matches[matchNum].player1.dx = 10;
      g_matches[matchNum].player1.isCharging = false;
      g_matches[matchNum].player1.jumpSpeed = 0;
      ++it;  // 충돌 시 반복자를 증가시킵니다.
    } else {
      ++it;  // 충돌이 발생하지 않았을 때도 반복자를 증가시킵니다.
    }
  }
  // player 2
  for (auto it = g_matches[matchNum].g_enemies.begin();
       it != g_matches[matchNum].g_enemies.end();) {
    if (g_matches[matchNum].player2.x >= it->x * GRID &&
        g_matches[matchNum].player2.x <= (it->x + 1) * GRID &&
        g_matches[matchNum].player2.y >= it->y * GRID &&
        g_matches[matchNum].player2.y <= (it->y + 1) * GRID) {
      g_matches[matchNum].player2.dx = 10;
      g_matches[matchNum].player2.isCharging = false;
      g_matches[matchNum].player2.jumpSpeed = 0;
      ++it;  // 충돌 시 반복자를 증가시킵니다.
    } else {
      ++it;  // 충돌이 발생하지 않았을 때도 반복자를 증가시킵니다.
    }
  }
}

void CheckItemPlayerCollisions(int matchNum) {
  // player 1
  for (auto it = g_matches[matchNum].g_items.begin();
       it != g_matches[matchNum].g_items.end();) {
    if (g_matches[matchNum].player1.x >= it->x * GRID &&
        g_matches[matchNum].player1.x <= (it->x + 1) * GRID &&
        g_matches[matchNum].player1.y >= it->y * GRID &&
        g_matches[matchNum].player1.y <= (it->y + 1) * GRID) {
      if (it->disable == false) {
        g_matches[matchNum].player1.EnhancedJumpPower = true;
        g_matches[matchNum].player1.isJumping = false;
        it->disable = true;
        it->interval = 60;
      }
    }
    ++it;
  }
  // player 2
  for (auto it = g_matches[matchNum].g_items.begin();
       it != g_matches[matchNum].g_items.end();) {
    if (g_matches[matchNum].player2.x >= it->x * GRID &&
        g_matches[matchNum].player2.x <= (it->x + 1) * GRID &&
        g_matches[matchNum].player2.y >= it->y * GRID &&
        g_matches[matchNum].player2.y <= (it->y + 1) * GRID) {
      if (it->disable == false) {
        g_matches[matchNum].player2.EnhancedJumpPower = true;
        g_matches[matchNum].player2.isJumping = false;
        it->disable = true;
        it->interval = 60;
      }
    }
    ++it;
  }
}

void CheckPlayerBulletCollisions(int matchNum) {
  // player1
  for (auto it = g_matches[matchNum].g_bullets.begin();
       it != g_matches[matchNum].g_bullets.end();) {
    if (it->x >= g_matches[matchNum].player1.x - PLAYER_SIZE &&
        it->x <= g_matches[matchNum].player1.x + PLAYER_SIZE &&
        it->y >= g_matches[matchNum].player1.y - PLAYER_SIZE &&
        it->y <= g_matches[matchNum].player1.y + PLAYER_SIZE) {
      // 플레이어를 뒤로 밀침
      g_matches[matchNum].player1.dx = 40;
      g_matches[matchNum].player1.isCharging = false;
      g_matches[matchNum].player1.jumpSpeed = 0;
      // 플레이어와 충돌 시 제거
      it = g_matches[matchNum].g_bullets.erase(it);
    } else {
      ++it;
    }
  }

  // player2
  for (auto it = g_matches[matchNum].g_bullets.begin();
       it != g_matches[matchNum].g_bullets.end();) {
    if (it->x >= g_matches[matchNum].player2.x - PLAYER_SIZE &&
        it->x <= g_matches[matchNum].player2.x + PLAYER_SIZE &&
        it->y >= g_matches[matchNum].player2.y - PLAYER_SIZE &&
        it->y <= g_matches[matchNum].player2.y + PLAYER_SIZE) {
      // 플레이어를 뒤로 밀침
      g_matches[matchNum].player2.dx = 40;
      g_matches[matchNum].player2.isCharging = false;
      g_matches[matchNum].player2.jumpSpeed = 0;
      // 플레이어와 충돌 시 제거
      it = g_matches[matchNum].g_bullets.erase(it);
    } else {
      ++it;
    }
  }
}

void CheckPlayersCollisions(int matchNum) {
  // 플레이어1과 플레이어2의 y 위치가 충분히 가깝다면 충돌 처리
  if (abs(g_matches[matchNum].player1.x - g_matches[matchNum].player2.x) <= PLAYER_SIZE) {
    if (g_matches[matchNum].player1.y <= g_matches[matchNum].player2.y - PLAYER_SIZE &&
             g_matches[matchNum].player1.y >= g_matches[matchNum].player2.y - 25)
    {
        g_matches[matchNum].player1.dy = -10;
        g_matches[matchNum].player2.dy = 5;
    }
    else if (g_matches[matchNum].player2.y <= g_matches[matchNum].player1.y - PLAYER_SIZE &&
             g_matches[matchNum].player2.y >= g_matches[matchNum].player1.y - 25)
    {
        g_matches[matchNum].player2.dy = -10;
        g_matches[matchNum].player1.dy = 5;
    }
    else if (abs(g_matches[matchNum].player1.y - g_matches[matchNum].player2.y) <= PLAYER_SIZE) {
      g_matches[matchNum].player1.dx =
            g_matches[matchNum].player2.dx < 0 ? -5 : 5;
      g_matches[matchNum].player2.dx =
        g_matches[matchNum].player2.dx < 0 ? 10 : -5;

      g_matches[matchNum].player1.isCharging = false;
      g_matches[matchNum].player1.jumpSpeed = 0;
      g_matches[matchNum].player2.isCharging = false;
      g_matches[matchNum].player2.jumpSpeed = 0;
    }
  }
}

void updateSendParam(int matchNum) {
  // player 1
  g_matches[matchNum].SPlayer1.x = g_matches[matchNum].player1.x;
  g_matches[matchNum].SPlayer1.y = g_matches[matchNum].player1.y;
  g_matches[matchNum].SPlayer1.face = g_matches[matchNum].player1.face;
  char acting = 0;
  acting = g_matches[matchNum].player1.isSliding ? '6'
           : (g_matches[matchNum].player1.isJumping &&
              g_matches[matchNum].player1.dy > 0)
               ? '4'
           : (g_matches[matchNum].player1.isJumping &&
              g_matches[matchNum].player1.dy < 0)
               ? '5'
           : (g_matches[matchNum].player1.isCharging &&
              g_matches[matchNum].player1.jumpSpeed <= -18)
               ? '3'
           : g_matches[matchNum].player1.isCharging ? '2'
           : g_matches[matchNum].player1.dx != 0    ? '1'
                                                    : '0';
  g_matches[matchNum].SPlayer1.acting = acting;  // 추후 충돌처리 이후 추가
  // player 2
  g_matches[matchNum].SPlayer2.x = g_matches[matchNum].player2.x;
  g_matches[matchNum].SPlayer2.y = g_matches[matchNum].player2.y;
  g_matches[matchNum].SPlayer2.face = g_matches[matchNum].player2.face;
  acting = g_matches[matchNum].player2.isSliding ? '6'
           : (g_matches[matchNum].player2.isJumping &&
              g_matches[matchNum].player2.dy > 0)
               ? '4'
           : (g_matches[matchNum].player2.isJumping &&
              g_matches[matchNum].player2.dy < 0)
               ? '5'
           : (g_matches[matchNum].player2.isCharging &&
              g_matches[matchNum].player2.jumpSpeed <= -18)
               ? '3'
           : g_matches[matchNum].player2.isCharging ? '2'
           : g_matches[matchNum].player2.dx != 0    ? '1'
                                                    : '0';
  g_matches[matchNum].SPlayer2.acting = acting;  // 추후 충돌처리 이후 추가
}
