#define SERVERPORT 9000
#define BUFSIZE 512

#include "common.h"
#include "map.h"
#include "sendParam.hpp"
#include <windows.h>  // windows 관련 함수 포함
#include <iostream>
#include <vector>

CRITICAL_SECTION cs;
HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

typedef struct Player {
  int x, y;
  int dx = 0, dy = 1;
  int jumpSpeed = 0;
  bool isCharging = false;
  bool isJumping = false;
  bool isSliding = false;
  bool slip = false;  // 미끄러지는 동안 계속 true
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
  char matchNum = 0;
  char p1 = 'a';
  char p2 = 'a';
  int mapNum;
  int score;
  std::vector<Item> g_items;
  std::vector<Enemy> g_enemies;
  std::vector<Bullet> g_bullets;

  bool operator==(const MATCH& other) {
    return client_sock[0] == other.client_sock[0] &&
           client_sock[1] == other.client_sock[1];
  }
};
std::vector<MATCH> g_matches;

// 충돌처리 함수
void initPlayer(int matchNum);
void updatePlayerD(int matchNum);
void applyGravity(int matchNum);
void movePlayer(int matchNum); // 플레이어 이동
// moveBullets();
// shootInterval++
// for (auto& item : g_items){interval, disable}
// ShootBullet
// IsNextColliding
// 오브젝트 충돌처리
void CheckCollisions(int matchNum);
void CheckEnemyPlayerCollisions(int matchNum);
void CheckItemPlayerCollisions(int matchNum);
void CheckPlayerBulletCollisions(int matchNum);
void updateSendParam(int matchNum);

// 매치를 삭제하는 함수
void closeSocketFunc(SOCKET client_sock, char matchNum, char playerNum) {
  // 디버그용 출력
  printf("%d매치의 %d번 플레이어의 연결이 종료됐습니다\n", matchNum, playerNum);

  std::vector<MATCH>::iterator iter = g_matches.begin();
  for (int i = 0; i < matchNum; i++) iter++;
  g_matches.erase(std::remove(g_matches.begin(), g_matches.end(), *iter),
                  g_matches.end());
  // 디버그용 출력
  printf("매치 원소 삭제\n");

  for (int i = 0; i < g_matches.size(); i++) {
    g_matches[i].matchNum = i;
  }

  // 바꾼 matchNum을 다른 스레드에도 적용시켜야함
  // -> 각 recv스레드에서 반복문이 시작될 때 자신의 매치 번호를 검사한다
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

  printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n", addr, ntohs(clientaddr.sin_port));
  
  while (1) {
    // 다른 스레드 작동중 대기
    WaitForSingleObject(hEvent, INFINITE);
    ResetEvent(hEvent);
    // 벡터의 유효한 범위 내에서, 현재 매치의 번호와, 매치[현재 매치번호]의 매치
    // 번호가 일치하는지 비교한다, 다르다면 감소
    
    while (matchNum >= 0 && matchNum < g_matches.size() &&
           g_matches[matchNum].matchNum != matchNum) {
        // 디버그옹 출력
      printf(
          "matchNum: %d\n matchNum번째 매치의 실제 매치 번호: %d\n일치하지 "
          "않음, matchNum감소\n",
          matchNum, g_matches[matchNum].matchNum);
      matchNum--;
    }
    // 디버그용 출력
    // printf("\nrecvThread 루프 시작, matchNum: %d, playerNum: %d\n", matchNum, playerNum); 
    // 데이터 받기
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
      
    // 받은 데이터 출력
    buf[retval] = '\0';
    if (playerNum == 0) {
      g_matches[matchNum].p1 = buf[0];
      //printf("%d매치 %d플레이어에게 받은 데이터: %c\n", matchNum, playerNum, buf[0]);    // 왼쪽 0, 오른쪽 1, 스페이스 입력때 한번, 땔때 한번
    }
    if (playerNum == 1) {
      g_matches[matchNum].p2 = buf[0];
      printf("[%s:%d] %c\n", addr, ntohs(clientaddr.sin_port), g_matches[matchNum].p2);
    }

    if (retval == SOCKET_ERROR) {
      err_display("send()");
      SetEvent(hEvent);
      break;
    } 
    // 이벤트 해제
    SetEvent(hEvent);
  }
  // 소켓 닫기
  closesocket(client_sock);
  printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n", addr, ntohs(clientaddr.sin_port));
  return 0;
}

DWORD WINAPI timerProcessClient(LPVOID lpParam) {
  // 타이머 생성
  int matchNum = (*(int*)lpParam);
  delete (int*)lpParam;
  HANDLE hTimer = CreateWaitableTimer(NULL, TRUE, NULL);
  if (hTimer == NULL) {
    printf("타이머 생성 실패\n");
    return 1;
  }

  // 타이머 간격을 설정 (1/30초)
  LARGE_INTEGER liDueTime;  // LARGE_INTEGER는 SetWaitableTimer에서 요구함
  liDueTime.QuadPart = -999900;

  while (true) {
    // 타이머 설정
    if (!SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, FALSE)) {
      printf("타이머 설정 실패\n");
      CloseHandle(hTimer);
      SetEvent(hEvent);
      return 1;
    }

    // 타이머 대기 시작
    // printf("타이머 대기 시작\n");
    DWORD waitResult = WaitForSingleObject(hTimer, INFINITE);
    if (waitResult != WAIT_OBJECT_0) {
      printf("타이머 대기 실패 또는 중단: %d\n", GetLastError());
      break;
    }
    // printf("타이머 대기 완료\n");

    //// 이벤트 대기
    //WaitForSingleObject(hEvent, INFINITE);
    //ResetEvent(hEvent);

    // 벡터의 유효한 범위 내에서, 현재 매치 번호와, 매치[현재 매치번호]의 매치
    // 번호가 일치하는 지 비교한다, 다르다면 감소
    while (!(matchNum < 0) && matchNum < g_matches.size() &&
           g_matches[matchNum].matchNum != matchNum) {
      // 디버그용 출력
      printf(
          "matchNum: %d\n matchNum번째 매치의 실제 매치 번호: %d\n일치하지 "
          "않음, matchNum감소\n",
          matchNum, g_matches[matchNum].matchNum);
      matchNum--;
    }

    EnterCriticalSection(&cs);
    // 플레이어 dx dy 변화
    updatePlayerD(matchNum);
    printf("%d, %d\n", g_matches[matchNum].player1.dx, g_matches[matchNum].player1.dy);
    applyGravity(matchNum);
    // 플레이어 이동
    // movePlayer(matchNum);
    // moveBullets();
    // shootInterval++
    // 아이템 재생성 코드
    // 총알 재생성 코드
    // 포탈 충돌처리
    // 오브젝트 충돌처리
    // sendParam업데이트
    updateSendParam(matchNum);
    printf("%d, %d\n", g_matches[matchNum].SPlayer1.x, g_matches[matchNum].SPlayer2.x);
    LeaveCriticalSection(&cs);
    // send 부분
    char sendBuf[1 + BUFSIZE];
    int sendSize = 1 + sizeof(sendParam::sendParam);

    for (int i = 0; i < 2; ++i) {
      if (g_matches[matchNum].client_sock[i] == NULL) {
        // printf("클라이언트 %d 소켓이 NULL입니다.\n", i);
        continue;
      }
      // printf("클라이언트 %d 소켓 확인: %d\n", i, g_matches[matchNum].client_sock[i]);
      if (i == 0) {
        sendBuf[0] = static_cast<std::int8_t>(
            sendParam::PKT_CAT::PLAYER_INFO);  // 패킷 타입 설정
        memcpy(sendBuf + 1, &g_matches[matchNum].SPlayer1,
               sizeof(sendParam::sendParam));  // 데이터 복사
      } else if (i == 1) {
        sendBuf[0] = static_cast<std::int8_t>(
            sendParam::PKT_CAT::PLAYER_INFO);  // 패킷 타입 설정
        memcpy(sendBuf + 1, &g_matches[matchNum].SPlayer2,
               sizeof(sendParam::sendParam));  // 데이터 복사
      }
      int retval = send(g_matches[matchNum].client_sock[i], sendBuf, sendSize, 0);
      if (retval == SOCKET_ERROR) {
        printf("클라이언트 %d에게 데이터 전송 실패: %d\n", i,
               WSAGetLastError());
      } else {
        // printf("클라이언트 %d에게 데이터 전송 성공: %d 바이트 전송됨\n", i, retval);
      }
    }
    // 이벤트 해제
    SetEvent(hEvent);
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
  int* matchNumParam;
  InitializeCriticalSection(&cs);

  while (1) {
    printf("서버 대기중...\n");
    addrlen = sizeof(clientaddr);
    // 여기서 rParam 할당 해서 생성 하고 rParam값 주고
    rParam = new recvParam{};
    matchNumParam = new int{};
    rParam->client_sock =
        accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
    if (rParam->client_sock == INVALID_SOCKET) {
      err_display("accept()");
      break;
    }
    // 매치 생성 조건 - 현재 매치가 없거나(0), 마지막 매치의 player가 다
    // 차있으면 생성 플레이어 1 생성 조건: 마지막 매치의 소켓0번이 비었으면 생성
    // 플레이어 2 생성 조건: 마지막 매치의 소켓1이 차있고 소켓2가 비었으면 생성
    // 타이머 생성 조건: 플레이어 1 생성할 때
    char addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
    /*printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n", addr,
           ntohs(clientaddr.sin_port));*/
    // 매치 생성
    if (g_matches.size() == 0 ||
        (!g_matches.empty() && g_matches.back().client_sock[0] != NULL &&
         g_matches.back().client_sock[1] != NULL))
      g_matches.push_back(MATCH());
    // 플레이어1 스레드, 타이머 스레드 생성
    if (g_matches.back().client_sock[0] == NULL) {
      rParam->playerNum = 0;
      rParam->matchNum = g_matches.size() - 1;
      *matchNumParam = g_matches.size() - 1;
      // g_matches의 클라이언트 소켓, 매치 넘버 업데이트
      g_matches.back().client_sock[0] = rParam->client_sock;
      g_matches.back().matchNum = g_matches.size() - 1;
      initPlayer(*matchNumParam);
      // 수신 스레드 생성
      g_matches.back().recvThread[0] =
          CreateThread(NULL, 0, RecvProcessClient, rParam, 0, NULL);
      // 디버그용 출력
      printf("%zu번 매치 대기중.. 클라이언트 수: %d\n", g_matches.size() - 1,
             1);
      // 타이머 스레드 생성
      g_matches.back().timerThread =
          CreateThread(NULL, 0, timerProcessClient, matchNumParam, 0, NULL);
    }
    // 플레이어2 스레드 생성
    
    else if (g_matches.back().client_sock[0] != NULL &&
             g_matches.back().client_sock[1] == NULL) {
      printf("2번 반복문 진입\n");
      rParam->playerNum = 1;
      rParam->matchNum = g_matches.size() - 1;
      g_matches.back().client_sock[1] = rParam->client_sock;
      // 디버그용 출력
      printf("%d번째 매치 %d번째 플레이어 스레드 생성\n", rParam->matchNum,
             rParam->playerNum);
      g_matches.back().recvThread[1] =
          CreateThread(NULL, 0, RecvProcessClient, rParam, 0, NULL);
      initPlayer(*matchNumParam);
    }
    // 이벤트 해제
    SetEvent(hEvent);
  }

  // 소켓 닫기
  closesocket(listen_sock);
  DeleteCriticalSection(&cs);
  // 윈속 종료
  WSACleanup();
  return 0;
}

void initPlayer(int matchNum) {
  g_matches[matchNum].player1.x = (MAP_WIDTH - 6) * GRID;
  g_matches[matchNum].player1.y = (MAP_HEIGHT - 4) * GRID;

  g_matches[matchNum].player2.x = (MAP_WIDTH - 8) * GRID;
  g_matches[matchNum].player2.y = (MAP_HEIGHT - 4) * GRID;
}

void updatePlayerD(int matchNum) {
  // player1 처리
  if (g_matches[matchNum].p1 == '0') {
    if (g_matches[matchNum].player1.dx >= -3) {
      g_matches[matchNum].player1.dx -= 1;
    }
  } else if (g_matches[matchNum].p1 == '1') {
    if (g_matches[matchNum].player1.dx <= 3) {
      g_matches[matchNum].player1.dx += 1;
    }
  } else if (g_matches[matchNum].p1 != '0' && g_matches[matchNum].p1 != '1') {
    // 왼쪽, 오른쪽 키가 모두 눌리지 않은 상태
    if (g_matches[matchNum].player1.dx > 0) {
      g_matches[matchNum].player1.dx -= 1;
    } else if (g_matches[matchNum].player1.dx < 0) {
      g_matches[matchNum].player1.dx += 1;
    }
  }
  g_matches[matchNum].p1 = 'a';
  // player2 처리
  if (g_matches[matchNum].p2 == 0) {
    if (g_matches[matchNum].player2.dx >= -3) {
      g_matches[matchNum].player2.dx -= 1;
    }
  } else if (g_matches[matchNum].p2 == 1) {
    if (g_matches[matchNum].player2.dx <= 3) {
      g_matches[matchNum].player2.dx += 1;
    }
  } else if (g_matches[matchNum].p2 != 0 && g_matches[matchNum].p2 != 1) {
    // 왼쪽, 오른쪽 키가 모두 눌리지 않은 상태
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
    g_matches[matchNum].player1.dy += GRAVITY;  // 중력 적용
  }
  if (g_matches[matchNum].player2.dy < 20) {
    g_matches[matchNum].player2.dy += GRAVITY;  // 중력 적용
  }
}

/* void movePlayer(int matchNum) {
  int newX = g_matches[matchNum].player1.x + g_matches[matchNum].player1.dx;
  int newY = g_player.y + g_player.dy;

  bool isVerticalCollision = IsColliding(map, g_player.x, newY);
  bool isHorizontalCollision = IsColliding(map, newX, g_player.y);
  bool isSlopeGoRightCollision =
      IsSlopeGoRightColliding(map, g_player.x, g_player.y);
  bool isSlopeGoLeftCollision =
      IsSlopeGoLeftColliding(map, g_player.x, g_player.y);

  // 수직 충돌 처리
  if (!isVerticalCollision) {
    g_player.y = newY;
    if (!g_player.EnhancedJumpPower) {
      g_player.isJumping = true;
    }
  } else {
    // 바닥 충돌 시 y축 위치 보정
    if (g_player.dy > 0) {
      while (!IsColliding(map, g_player.x, g_player.y + 1)) {
        g_player.y += 1;
      }
    }
    g_player.dy = 0;  // 충돌 후 y축 속도 초기화
    g_player.isJumping = false;
    g_player.isSliding = false;
  }

  // 수평 충돌 처리
  if (!isHorizontalCollision) {
    g_player.x = newX;
  } else {
    g_player.dx = 0;  // 충돌 후 x축 속도 초기화
  }

  if (isSlopeGoRightCollision) {
    g_player.isSliding = true;

    g_player.dy = 1;  // 경사면 위에서 미끄러짐 속도
    g_player.dx = 3;  // 오른쪽 아래로 미끄러짐
    newX = g_player.x + g_player.dx;
    newY = g_player.y + g_player.dy;
    g_player.x = newX;
    g_player.y = newY;
  }

  if (isSlopeGoLeftCollision) {
    g_player.isSliding = true;

    g_player.dy = 1;   // 경사면 위에서 미끄러짐 속도
    g_player.dx = -3;  // 오른쪽 아래로 미끄러짐
    newX = g_player.x + g_player.dx;
    newY = g_player.y + g_player.dy;
    g_player.x = newX;
    g_player.y = newY;
  }
}*/
// moveBullets();
// shootInterval++
// 아이템 재생성 코드
// 총알 재생성 코드
// 포탈 충돌처리

void updateSendParam(int matchNum) {
  // player 1
  g_matches[matchNum].SPlayer1.x = g_matches[matchNum].player1.x;
  g_matches[matchNum].SPlayer1.y = g_matches[matchNum].player1.y;
  g_matches[matchNum].SPlayer1.acting = 0;  // 추후 충돌처리 이후 추가
                                            // player 2
  g_matches[matchNum].SPlayer2.x = g_matches[matchNum].player2.x;
  g_matches[matchNum].SPlayer2.y = g_matches[matchNum].player2.y;
  g_matches[matchNum].SPlayer2.acting = 0;  // 추후 충돌처리 이후 추가
}

void CheckCollisions(int matchNum) {
  CheckEnemyPlayerCollisions(matchNum);
  CheckItemPlayerCollisions(matchNum);
  CheckPlayerBulletCollisions(matchNum);
  //CheckPlayersCollisions(matchNum);
}
void CheckEnemyPlayerCollisions(int matchNum) {}
void CheckItemPlayerCollisions(int matchNum) {}
void CheckPlayerBulletCollisions(int matchNum) {}
