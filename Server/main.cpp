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
	// match[���ο��� �˷��� ����].client_socket[���ο��� �˷��ٿ���] = client_sock;
	struct sockaddr_in clientaddr;
	char addr[INET_ADDRSTRLEN];
	int addrlen;
	char buf[BUFSIZE + 1];

	// Ŭ���̾�Ʈ ���� ���
	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (struct sockaddr*)&clientaddr, &addrlen);
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));

	EnterCriticalSection(&cs);
	printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", addr, ntohs(clientaddr.sin_port));
	LeaveCriticalSection(&cs);

	while (1) {
		// ������ �ޱ�
		retval = recv(client_sock, buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		else if (retval == 0)
			break;

		// ���� ������ ���
		buf[retval] = '\0';
		EnterCriticalSection(&cs);
		printf("[%s:%d] %s\n", addr, ntohs(clientaddr.sin_port), buf);
		// ���ο��� �˷��ذſ� ���� 
		// match[���ο��� �˷���].p? = buf;
		LeaveCriticalSection(&cs);

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

// ���1. CreateWaitableTimer >> ������ ª�� �ֱ�� ���� ��Ȯ���� �ʿ��� ���
DWORD WINAPI clientProcess(LPVOID lpParam) {
	// Ÿ�̸� ����
	int matchNum = (int)lpParam;
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
		g_matches[matchNum].player1.x += g_matches[matchNum].player1.dx;
		g_matches[matchNum].player2.x += g_matches[matchNum].player2.dx;
		// �ٸ� ������ ������Ʈ ���� �߰� �ؾ���    
		LeaveCriticalSection(&cs);
		//send �߰��ؾ���
		printf("Ÿ�̸ӽ����� ����\n");
		// �ʿ信 ���� Ÿ�̸� �ߴ� ������ �߰�.
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
		// clinetCount: ���� Ŭ���̾�Ʈ ��, 2��° Ŭ���̾�Ʈ�� ��ġ�� �߰��Ǹ� �ٽ� 0, ��ġ ������ ��ġ �� �÷��̾� �� �Ǵ�
		// matchCount: ���� ��ġ ��, 2��° Ŭ���̾�Ʈ�� ��ġ�� �߰��Ǹ� +1, ��ġ ��ȣ�� ����
		// ��ġ�� �������� matchNum ���� �ʿ�, Ŭ���̾�Ʈ ���� ���� �� recv �ۼ��� Ȯ���ϰ� �߰��� ����
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