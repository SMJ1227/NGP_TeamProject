#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <stdio.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <vector>

#pragma comment(lib, "ws2_32")
#define SERVERPORT 9000
#define BUFSIZE 1024

char clientCount = 0;
char matchCount = 0;
CRITICAL_SECTION cs;

struct Player {
	int x, y;
	int dx, dy;
	int jumpSpeed;
	bool isCharging;
	bool isJumping;
	bool isSliding;
	bool damaged;
	bool face;
	bool EnhancedJumpPower;
};

struct Item {
	int x, y;
	int interval;
	bool disable;
};

struct Enemy {
	int x, y;
};

struct Bullet {
	int x, y;
	int dx, dy;
};

struct recvParam {
	SOCKET client_sock = NULL;
	char matchNum = NULL;
	char playerNum = NULL;
};

typedef struct MATCH {
	SOCKET client_sock[2]{ NULL, NULL };
	HANDLE recvThread[2]{ NULL, NULL };
	HANDLE timerThread;
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

void err_display(const char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char*)&lpMsgBuf, 0, NULL);

	printf("[%s] %s\n", msg, (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}
void err_quit(const char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char*)&lpMsgBuf, 0, NULL);

	MessageBoxA(NULL, (const char*)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

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
		}
		else if (retval == 0)
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
	int matchNum = (int)lpParam;
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

int main() {
	int retval;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("WSAstartup failed\n");
		return 1;
	}

	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	struct sockaddr_in clientaddr;
	int addrlen;
	HANDLE hThread;
	recvParam rParam;

	while (1) {
		addrlen = sizeof(clientaddr);
		rParam.client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (rParam.client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}
		// clinetCount: 현재 클라이언트 수, 2번째 클라이언트가 매치에 추가되면 다시 0, 매치 생성과 매치 내 플레이어 수 판단
		// matchCount: 현재 매치 수, 2번째 클라이언트가 매치에 추가되면 +1, 매치 번호로 전달
		// 매치가 없어지면 matchNum 조정 필요, 클라이언트 메인 만든 후 recv 송수신 확인하고 추가할 예정
		if (clientCount == 0) g_matches.push_back(MATCH());
		if (g_matches[matchCount].client_sock[clientCount] == NULL) {
			rParam.playerNum = 0;
			rParam.matchNum = matchCount;
			g_matches[matchCount].recvThread[0] = CreateThread(NULL, 0, RecvProcessClient, &rParam, 0, NULL);
			clientCount++;
			hThread = CreateThread(NULL, 0, clientProcess, &matchCount, 0, NULL);
		}
		else if (g_matches[matchCount].client_sock[0] != NULL && g_matches[matchCount].client_sock[1] == NULL) {
			rParam.playerNum = 1;
			rParam.matchNum = matchCount;
			g_matches[matchCount].recvThread[1] = CreateThread(NULL, 0, RecvProcessClient, &rParam, 0, NULL);
			matchCount++;
			clientCount = 0;
		}
	}
}