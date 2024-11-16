#include <WS2tcpip.h>
#include <atlImage.h>
#include <math.h>
#include <time.h>

//
#include <windows.h>

#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <syncstream>
#include <vector>

#include "network_util.hpp"
#include "resource.h"

#pragma comment(lib, "ws2_32")

const int WINDOW_WIDTH = 480;
const int WINDOW_HEIGHT = 600;
const int GRID = 40;
const int MAP_WIDTH = 13;
const int MAP_HEIGHT = 33;
const int BOARD_WIDTH = MAP_WIDTH * GRID;
const int BOARD_HEIGHT = MAP_HEIGHT * GRID;
const int PLAYER_SIZE = 20;
const double M_PI = 3.141592;
const int GRAVITY = 1;  // �߷� ���

int map_num = 0;
int tile_num = 0;
int map0[MAP_HEIGHT][MAP_WIDTH] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 1, 1, 1, 1, 1, 1, 6, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0}, {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 5, 1, 1, 1, 1, 1, 1, 0}, {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 0}, {0, 1, 1, 3, 2, 1, 1, 1, 1, 3, 0, 0},
    {0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0}, {0, 1, 1, 0, 0, 0, 2, 1, 1, 1, 1, 0},
    {0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0}, {0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0},
    {0, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0}, {0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 3, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0}, {0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0},
    {0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0}, {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0}, {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 0}, {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0}, {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0}, {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 2, 1, 1, 1, 1, 1, 5, 1, 1, 1, 0}, {0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 3, 0},
    {0, 0, 0, 2, 1, 1, 1, 1, 1, 3, 0, 0}, {0, 4, 0, 0, 2, 1, 1, 1, 3, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};
int tile0[MAP_HEIGHT][MAP_WIDTH] = {
    {1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3},
    {4, 0, 0, 0, 0, 0, 0, 18, 0, 0, 0, 6},
    {4, 0, 0, 0, 0, 14, 15, 16, 0, 0, 0, 6},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 6},
    {4, 0, 0, 10, 12, 0, 0, 0, 0, 10, 11, 6},
    {4, 0, 0, 4, 13, 3, 0, 0, 0, 7, 8, 6},
    {4, 0, 0, 4, 5, 13, 12, 0, 0, 0, 0, 6},
    {4, 0, 0, 4, 5, 5, 13, 3, 0, 0, 0, 6},
    {4, 0, 0, 7, 8, 8, 8, 9, 15, 0, 0, 6},
    {4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6},
    {4, 14, 15, 15, 16, 0, 15, 0, 0, 0, 10, 6},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 11, 6},
    {4, 0, 0, 0, 0, 0, 0, 0, 15, 0, 0, 6},
    {4, 0, 0, 15, 0, 15, 0, 0, 0, 0, 0, 6},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6},
    {4, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6},
    {4, 0, 0, 15, 0, 0, 0, 0, 15, 0, 0, 6},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6},
    {4, 0, 0, 0, 0, 15, 0, 0, 0, 0, 0, 6},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6},
    {4, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6},
    {4, 13, 12, 0, 0, 0, 0, 0, 0, 0, 10, 6},
    {4, 5, 13, 12, 0, 0, 0, 0, 0, 10, 11, 6},
    {4, 0, 5, 13, 12, 0, 0, 0, 10, 11, 5, 6},
    {17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17},
};
int map1[MAP_HEIGHT][MAP_WIDTH] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 6, 0}, {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0}, {0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0}, {0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0}, {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 0, 0, 2, 1, 1, 1, 1, 3, 0, 0, 0}, {4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0}, {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0}, {0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0}, {0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 0}, {0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0}, {0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0},
    {0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0}, {0, 1, 1, 1, 1, 5, 1, 1, 1, 1, 1, 0},
    {0, 1, 1, 3, 0, 0, 1, 1, 1, 1, 1, 0}, {4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 3, 0, 0}, {0, 0, 0, 1, 1, 1, 1, 1, 3, 0, 0, 0},
    {0, 1, 1, 1, 1, 1, 1, 3, 0, 0, 0, 0}, {0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
int tile1[MAP_HEIGHT][MAP_WIDTH] = {{1, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 2},
                                    {7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
                                    {7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 13, 8},
                                    {7, 0, 9, 9, 9, 9, 9, 9, 9, 9, 9, 8},
                                    {7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
                                    {7, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0, 8},
                                    {7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
                                    {7, 9, 9, 0, 9, 0, 9, 0, 0, 0, 0, 8},
                                    {7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
                                    {7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
                                    {7, 9, 9, 11, 0, 0, 0, 0, 10, 9, 9, 8},
                                    {7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
                                    {7, 0, 0, 0, 0, 9, 9, 0, 0, 0, 0, 8},
                                    {7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
                                    {7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
                                    {7, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0, 8},
                                    {7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
                                    {7, 0, 0, 0, 0, 9, 9, 0, 0, 12, 0, 8},
                                    {7, 0, 0, 0, 0, 0, 0, 0, 10, 0, 0, 8},
                                    {7, 0, 0, 0, 0, 0, 0, 0, 9, 0, 0, 8},
                                    {7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 8},
                                    {7, 0, 0, 0, 0, 0, 0, 0, 9, 0, 0, 8},
                                    {7, 0, 0, 0, 0, 0, 0, 9, 0, 0, 0, 8},
                                    {7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
                                    {7, 0, 0, 10, 9, 9, 0, 0, 0, 0, 0, 8},
                                    {7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 8},
                                    {7, 0, 0, 0, 0, 0, 0, 0, 0, 10, 9, 8},
                                    {7, 9, 9, 0, 0, 0, 0, 0, 10, 9, 9, 8},
                                    {7, 0, 0, 0, 0, 0, 0, 10, 9, 9, 9, 8},
                                    {7, 0, 0, 0, 0, 0, 0, 9, 9, 9, 9, 8},
                                    {3, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 4}};
int map2[MAP_HEIGHT][MAP_WIDTH] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 6, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 5, 1, 1, 0}, {0, 1, 5, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0}, {0, 1, 1, 1, 1, 1, 5, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0}, {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 5, 1, 0}, {0, 1, 1, 1, 1, 5, 1, 1, 1, 1, 1, 0},
    {0, 1, 5, 1, 1, 1, 1, 1, 1, 3, 1, 0}, {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 0}, {0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 1, 1, 5, 1, 1, 1, 0}, {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 1, 5, 1, 1, 1, 1, 1, 1, 1, 1, 0}, {0, 1, 1, 1, 1, 5, 1, 1, 1, 1, 1, 0},
    {0, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 0}, {0, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 5, 1, 1, 0}, {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {4, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 0}, {0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0},
    {4, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0}, {0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 5, 0},
    {4, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0}, {0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 3, 0},
    {0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0}, {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};
int tile2[MAP_HEIGHT][MAP_WIDTH] = {
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2}, {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 2},
    {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2}, {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2},
    {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2}, {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2},
    {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2}, {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2},
    {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2}, {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2},
    {2, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 2}, {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2},
    {2, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 2}, {2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 2},
    {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2}, {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2},
    {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2}, {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2},
    {2, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 2}, {2, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 2},
    {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2}, {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2},
    {2, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 2}, {2, 0, 0, 0, 0, 0, 3, 1, 0, 0, 0, 2},
    {2, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2}, {2, 0, 0, 3, 0, 0, 0, 2, 0, 0, 0, 2},
    {2, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2}, {2, 3, 0, 0, 0, 0, 0, 2, 0, 0, 5, 2},
    {2, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2}, {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

using namespace std;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

HINSTANCE g_hInst;
LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"JumpKing";

WSADATA g_wsa;
std::string_view const g_server_address{"127.0.0.1"};
int const g_server_port = 9000;
HANDLE g_hSendThread{};
HANDLE g_hRecvThread{};

// ���� ����
struct Player {
  int x, y;
  bool isWarking;
  bool isCharging;
  bool isJumping;
  bool isFalling;
  bool isSliding;
  bool damaged;
  bool face;
  bool EnhancedJumpPower;
} g_player;
Player otherPlayer;

struct Item {
  int x, y;
  int interval;
  bool disable;
};
vector<Item> g_items;

struct Enemy {
  int x, y;
};
vector<Enemy> g_enemies;

struct Bullet {
  int x, y;
  int dx, dy;
};
vector<Bullet> g_bullets;

void ProcessKeyboard();
void DrawSnowBg(HDC hDC);
void DrawDesertBg(HDC hDC);
void DrawSnowTile(HDC hDC);
void DrawDesertTile(HDC hDC);
void DrawForestBg(HDC hDC);
void DrawForestTile(HDC hDC);
void InitMap(int dst[MAP_HEIGHT][MAP_WIDTH], int src[MAP_HEIGHT][MAP_WIDTH]);
void InitPlayer();
void DrawSprite(HDC hDC, const int& x, const int& y, const int& width,
                const int& height);
void InitItems(int map[MAP_HEIGHT][MAP_WIDTH]);
void GenerateItem(int x, int y, int num);
void DrawItem(HDC hDC);
void DeleteAllItems();
void InitEnemy(int map[MAP_HEIGHT][MAP_WIDTH]);
void GenerateEnemy(int x, int y);
void DrawEnemies(HDC hDC);
void DeleteAllEnemies();
void ShootBullet();
void DrawBullets(HDC hDC);
void DeleteAllBullets();

// WinMain �Լ�
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpszCmdParam, int nCmdShow) {
  srand((unsigned int)time(NULL));

  // ���� �ʱ�ȭ
  if (WSAStartup(MAKEWORD(2, 2), &g_wsa) != 0) {
    std::exit(-1);
  };
  HWND hWnd;
  MSG Message;
  WNDCLASSEX WndClass;
  g_hInst = hInstance;

  WndClass.cbSize = sizeof(WndClass);
  WndClass.style = CS_HREDRAW | CS_VREDRAW;
  WndClass.lpfnWndProc = (WNDPROC)WndProc;
  WndClass.cbClsExtra = 0;
  WndClass.cbWndExtra = 0;
  WndClass.hInstance = hInstance;
  WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  WndClass.hCursor = LoadCursor(NULL, IDC_HAND);
  WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
  WndClass.lpszMenuName = MAKEINTRESOURCE(NULL);
  WndClass.lpszClassName = lpszClass;
  WndClass.hIconSm = LoadIcon(NULL, IDI_QUESTION);
  RegisterClassEx(&WndClass);

  hWnd = CreateWindow(lpszClass, lpszWindowName, WS_OVERLAPPEDWINDOW, 0, 0,
                      WINDOW_WIDTH + 16, WINDOW_HEIGHT + 59, NULL, (HMENU)NULL,
                      hInstance, NULL);
  ShowWindow(hWnd, nCmdShow);
  UpdateWindow(hWnd);

  while (1) {
    if (PeekMessage(&Message, NULL, 0, 0, PM_REMOVE)) {
      if (Message.message == WM_QUIT) break;
    }
    // #1 ���콺 ���õ� �޼����� �����ϴ� ù��° ���
    // if (Message.message == WM_MOUSEMOVE || Message.message == WM_LBUTTONDOWN
    // || Message.message == WM_RBUTTONDOWN) {
    //    // ���콺 �޽��� ����
    //    continue;
    //}
    TranslateMessage(&Message);
    DispatchMessage(&Message);
  }

  WSACleanup();
  return Message.wParam;
}

std::ofstream clientlog{"client_send.log"};
std::osyncstream wow{clientlog};

DWORD WINAPI SendClient(LPVOID lp_param) {
  SOCKET server_sock = (SOCKET)lp_param;
  int return_value{};
  bool is_pressed_left{false};
  bool is_pressed_right{false};
  bool is_pressed_space{false};

  std::vector<char> buffer{};
  buffer.reserve(3);

  while (true) {
    // �Է� ���� Ȯ��
    is_pressed_left = (GetAsyncKeyState(VK_LEFT) & 0x8000) != 0;
    is_pressed_right = (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0;
    is_pressed_space = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

    // ���� ����
    buffer.clear();

    // ��ȿ �Է� ó��
    if (!(is_pressed_left && is_pressed_right)) {
      if (is_pressed_left) {
        buffer.push_back('0');
      }
      if (is_pressed_right) {
        buffer.push_back('1');
      }
    }
    if (is_pressed_space) {
      buffer.push_back(' ');
    }

    // ���� ���� ������ �ѱ�
    if (buffer.empty()) {
      std::this_thread::yield();
      continue;
    }

    // ���� �� �α�
    return_value = send(server_sock, buffer.data(), buffer.size(), 0);
    std::println(wow, "res {} = {}:{}", return_value, std::string_view{buffer},
                 buffer.size());
    wow.emit();

    switch (return_value) {
      case SOCKET_ERROR: {
        err_quit("result of send : sock error");
        break;
      }
      case 0: {
        err_quit("result of send : server disconnect");
        break;
      }
    }

    // �Է� ���
    Sleep(1000 / 60);
  }
  return 0;
}
DWORD WINAPI RecvClient(LPVOID lp_param) {

    return 0; 
}

//--- CImage ���� ���� ����
CImage Snowtile;
CImage Snowbg;
CImage Desertbg;
CImage Deserttile;
CImage Forestbg;
CImage Foresttile;
CImage cannon;
CImage item_EnhanceJump;
CImage portal;
CImage startImage;
CImage endImage;
HBITMAP spriteSheet;
HBITMAP spriteSheetMask;

static int map[MAP_HEIGHT][MAP_WIDTH];
static int shootInterval = 0;
static int spriteX = 0;
static int spriteY = 0;
static int spriteWidth = 30;
static int spriteHeight = 0;

// Ÿ�̸� �ݹ�
void CALLBACK TimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
  switch (idEvent) {
    case 2: {
      if ((GetAsyncKeyState('s') & 0x8000) ||
          (GetAsyncKeyState('S') & 0x8000)) {
        // ���� �õ�
        int return_value{};
        SOCKET server_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (INVALID_SOCKET == server_sock) {
          err_quit("socket() ȣ��");
        }

        sockaddr_in serveraddr{.sin_family = AF_INET,
                               .sin_port = htons(g_server_port)};
        return_value =
            inet_pton(AF_INET, g_server_address.data(), &serveraddr.sin_addr);
        if (SOCKET_ERROR == return_value) {
          err_quit("��ȿ���� ���� �ּҰ� �ԷµǾ����ϴ�.");
        }

        return_value =
            connect(server_sock, reinterpret_cast<sockaddr const*>(&serveraddr),
                    sizeof(serveraddr));
        if (return_value == SOCKET_ERROR) {
          err_quit("connect() ȣ��");
        }

        // ���� ������ ClientSend, CliendRecv ����
        g_hSendThread =
            CreateThread(NULL, 0, SendClient, (LPVOID)server_sock, 0, NULL);
        g_hRecvThread =
            CreateThread(NULL, 0, RecvClient, (LPVOID)server_sock, 0, NULL);


        /*map_num = 1;
        InitPlayer();
        InitMap(map, map0);
        InitEnemy(map);
        InitItems(map);*/
        KillTimer(hWnd, 2);
      }
      break;
    }
  }

  InvalidateRect(hWnd, NULL, FALSE);
}

// ���� �Լ�

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
                         LPARAM lParam) {
  PAINTSTRUCT ps;
  HDC hDC;
  HDC mDC;
  HBITMAP hBitmap;
  RECT rt;

  static int playerFrameIndex = 0;

  switch (message) {
    case WM_CREATE:
      Snowtile.Load(L"snowtile.png");
      Snowbg.Load(L"SnowBg.png");
      cannon.Load(L"Cannon.png");
      Desertbg.Load(L"desertbg_sand4.png");
      Deserttile.Load(L"deserttiles.png");
      Forestbg.Load(L"forestbg2.png");
      Foresttile.Load(L"foresttiles2.png");
      item_EnhanceJump.Load(L"wing.png");
      portal.Load(L"portal.png");
      startImage.Load(L"start_title.png");
      endImage.Load(L"clear.png");
      spriteSheet =
          (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(PLAYER_SPRITE));
      spriteSheetMask =
          (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(PLAYER_SPRITE_MASK));
      /*
            �÷��̾�2�� spriteSheet, mask ����
      */
      SetTimer(hWnd, 2, 1000 / 60, (TIMERPROC)TimerProc);
      break;
    case WM_CHAR:
      switch (wParam) {
        case 'Q':
        case 'q':
          PostQuitMessage(0);
          break;
      }
      break;
    case WM_PAINT: {
      hDC = BeginPaint(hWnd, &ps);
      GetClientRect(hWnd, &rt);
      mDC = CreateCompatibleDC(hDC);
      hBitmap = CreateCompatibleBitmap(hDC, BOARD_WIDTH, BOARD_HEIGHT);
      SelectObject(mDC, (HBITMAP)hBitmap);

      //--- ��� �׸��⸦ �޸� DC���Ѵ�.  ---> �ٲ� �κ�: CImage ������
      // ���������� �����Ͽ� �Լ��� ���ڷ� ������ �ʵ��� �Ѵ�.
      
      // mapNum, map, CIMAGE, SPRITE, vector(enemy, bullet, item)�� ������, ����ȭ ���� �ʿ�
      if (map_num == 0)
        startImage.Draw(mDC, 0, 0, GRID * 12, GRID * 15);
      else if (map_num == 1) {
        DrawSnowBg(mDC);
        DrawSnowTile(mDC);
        DrawEnemies(mDC);
        DrawBullets(mDC);
        DrawSprite(mDC, spriteX, spriteY, spriteWidth, spriteHeight);
        // DrawSprite(mDC, �÷��̾�2�� spriteX, spriteY, spriteWidth, spriteHeight);
        DrawItem(mDC);
      } else if (map_num == 2) {
        DrawDesertBg(mDC);
        DrawDesertTile(mDC);
        DrawEnemies(mDC);
        DrawBullets(mDC);
        DrawSprite(mDC, spriteX, spriteY, spriteWidth, spriteHeight);
        // DrawSprite(mDC, �÷��̾�2�� spriteX, spriteY, spriteWidth, spriteHeight);
        DrawItem(mDC);
      } else if (map_num == 3) {
        DrawForestBg(mDC);
        DrawForestTile(mDC);
        DrawEnemies(mDC);
        DrawBullets(mDC);
        DrawSprite(mDC, spriteX, spriteY, spriteWidth, spriteHeight);
        // DrawSprite(mDC, �÷��̾�2�� spriteX, spriteY, spriteWidth, spriteHeight);
        DrawItem(mDC);
      } else if (map_num == 4) {
        endImage.Draw(mDC, BOARD_WIDTH - GRID * 13, BOARD_HEIGHT - GRID * 17,
                      GRID * 12, GRID * 15);
      }

      // �޸� DC���� ȭ�� DC�� �׸��� ����
      // #1 �� ��ü�� �׸���
      // BitBlt(hDC, 0, 0, BOARD_WIDTH, BOARD_HEIGHT, mDC, 0, 0, SRCCOPY);
      // #2 �÷��̾� �ֺ��� ������ ������ ��ü�� Ȯ��
      int stretchWidth = rt.right;
      int stretchHeight = rt.bottom;
      int sourceWidth = WINDOW_WIDTH;
      int sourceHeight = WINDOW_HEIGHT;
      int sourceX = g_player.x - WINDOW_WIDTH / 2;
      if (sourceX <= 0) {
        sourceX = 0;
      }
      if (g_player.x + WINDOW_WIDTH / 2 >= WINDOW_WIDTH) {
        sourceX = WINDOW_WIDTH - sourceWidth;
      }
      int sourceY = g_player.y - WINDOW_HEIGHT / 2;
      if (sourceY - sourceHeight >= 0) {
        sourceY = sourceHeight + GRID;
      }
      if (sourceY <= 0) {
        sourceY = 0;
      }

      StretchBlt(hDC, 0, 0, stretchWidth, stretchHeight, mDC, sourceX, sourceY,
                 sourceWidth, sourceHeight, SRCCOPY);

      DeleteDC(mDC);
      DeleteObject(hBitmap);
      EndPaint(hWnd, &ps);
      break;
    }
    case WM_DESTROY:
      Snowtile.Destroy();
      Snowbg.Destroy();
      cannon.Destroy();
      Desertbg.Destroy();
      Deserttile.Destroy();
      Forestbg.Destroy();
      Foresttile.Destroy();
      item_EnhanceJump.Destroy();
      portal.Destroy();
      startImage.Destroy();
      endImage.Destroy();
      DeleteObject(spriteSheet);
      DeleteObject(spriteSheetMask);
      // �÷��̾�2 ��������Ʈ DeleteObject
      PostQuitMessage(0);
      break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}

// Ű�Է�
bool spaceKeyReleased = true;

// ��
void DrawSnowTile(HDC hDC) {
  // ĭ�� 96x96
  for (int y = 0; y < MAP_HEIGHT; y++) {
    for (int x = 0; x < MAP_WIDTH; x++) {
      int tileType = tile0[y][x];
      switch (tileType) {
        case 1:
          Snowtile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 0, 0, 96, 96);
          break;
        case 2:
          Snowtile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 96, 0, 96, 96);
          break;
        case 3:
          Snowtile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 96 * 2, 0, 96, 96);
          break;
        case 4:
          Snowtile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 0, 96, 96, 96);
          break;
        case 5:
          Snowtile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 96, 96, 96, 96);
          break;
        case 6:
          Snowtile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 96 * 2, 96, 96,
                        96);
          break;
        case 7:
          Snowtile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 0, 96 * 2, 96, 96);
          break;
        case 8:
          Snowtile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 96, 96 * 2, 96,
                        96);
          break;
        case 9:
          Snowtile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 96 * 2, 96 * 2, 96,
                        96);
          break;
        case 10:
          Snowtile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 96 * 3, 0, 96, 96);
          break;
        case 11:
          Snowtile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 96 * 3, 96, 96,
                        96);
          break;
        case 12:
          Snowtile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 96 * 4, 0, 96, 96);
          break;
        case 13:
          Snowtile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 96 * 4, 96, 96,
                        96);
          break;
        case 14:
          Snowtile.Draw(hDC, x * GRID, y * GRID, GRID, GRID * 2, 96 * 5, 0, 96,
                        96);
          break;
        case 15:
          Snowtile.Draw(hDC, x * GRID, y * GRID, GRID, GRID * 2, 96 * 6, 0, 96,
                        96);
          break;
        case 16:
          Snowtile.Draw(hDC, x * GRID, y * GRID, GRID, GRID * 2, 96 * 7, 0, 96,
                        96);
          break;
        case 17:
          Snowtile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 96 * 8, 0, 96, 96);
          break;
        case 18:
          portal.TransparentBlt(hDC, x * GRID, y * GRID, GRID, GRID,
                                RGB(0, 255, 0));
          break;
      }
    }
  }
}
void DrawDesertTile(HDC hDC) {
  // ĭ�� 32x32
  for (int y = 0; y < MAP_HEIGHT; y++) {
    for (int x = 0; x < MAP_WIDTH; x++) {
      int tileType = tile1[y][x];
      switch (tileType) {
        case 1:
          Deserttile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 32 * 5, 0, 32,
                          32);
          break;
        case 2:
          Deserttile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 32 * 4, 0, 32,
                          32);
          break;
        case 3:
          Deserttile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 32 * 7, 0, 32,
                          32);
          break;
        case 4:
          Deserttile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 32 * 6, 0, 32,
                          32);
          break;
        case 5:
          Deserttile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 32 * 8, 0, 32,
                          32);
          break;
        case 6:
          Deserttile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 32 * 9, 0, 32,
                          32);
          break;
        case 7:
          Deserttile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 32 * 10, 0, 32,
                          32);
          break;
        case 8:
          Deserttile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 32 * 11, 0, 32,
                          32);
          break;
        case 9:
          Deserttile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 0, 0, 32, 32);
          break;
        case 10:
          Deserttile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 32 * 2, 0, 32,
                          32);
          break;
        case 11:
          Deserttile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 32 * 3, 0, 32,
                          32);
          break;
        case 12:
          Deserttile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 32 * 1, 0, 32,
                          32);
          break;
        case 13:
          portal.TransparentBlt(hDC, x * GRID, y * GRID, GRID, GRID,
                                RGB(0, 255, 0));
          break;
      }
    }
  }
}
void DrawSnowBg(HDC hDC) {
  Snowbg.StretchBlt(hDC, -GRID / 2, 0, BOARD_WIDTH, BOARD_HEIGHT, SRCCOPY);
}
void DrawDesertBg(HDC hDC) {
  Desertbg.StretchBlt(hDC, -GRID / 2, 0, BOARD_WIDTH, BOARD_HEIGHT, SRCCOPY);
}
void DrawForestBg(HDC hDC) {
  Forestbg.StretchBlt(hDC, -GRID / 2, 0, BOARD_WIDTH, BOARD_HEIGHT, SRCCOPY);
}
void DrawForestTile(HDC hDC) {
  // ĭ�� 64x64
  for (int y = 0; y < MAP_HEIGHT; y++) {
    for (int x = 0; x < MAP_WIDTH; x++) {
      int tileType = tile2[y][x];
      switch (tileType) {
        case 1:
          Foresttile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 0, 64, 64, 64);
          break;
        case 2:
          Foresttile.Draw(hDC, x * GRID, y * GRID, GRID, GRID, 0, 192, 64, 64);
          break;
        case 3:
          Foresttile.TransparentBlt(hDC, x * GRID, y * GRID, GRID, GRID, 192,
                                    64, 64, 64, RGB(255, 255, 255));
          break;
        case 4:
          Foresttile.TransparentBlt(hDC, x * GRID, y * GRID, GRID, GRID, 256, 0,
                                    64, 64, RGB(255, 255, 255));
          break;
        case 5:
          Foresttile.TransparentBlt(hDC, x * GRID, y * GRID, GRID, GRID, 256,
                                    65, 64, 64, RGB(255, 255, 255));
          break;
        case 6:
          portal.TransparentBlt(hDC, x * GRID, y * GRID, GRID, GRID,
                                RGB(0, 255, 0));
          break;
      }
    }
  }
}
void InitMap(int dst[MAP_HEIGHT][MAP_WIDTH], int src[MAP_HEIGHT][MAP_WIDTH]) {
  for (int i = 0; i < MAP_HEIGHT; i++) {
    for (int j = 0; j < MAP_WIDTH; j++) {
      dst[i][j] = src[i][j];
    }
  }
}
// �÷��̾�
void InitPlayer() {
  g_player.x = (MAP_WIDTH - 7) * GRID;
  g_player.y = (MAP_HEIGHT - 4) * GRID;
  g_player.isCharging = false;
  g_player.isJumping = false;
  g_player.damaged = false;
  g_player.face = 0;
}

void DrawSprite(HDC hDC, const int& x, const int& y, const int& width,
                const int& height) {
  HDC hmemDC = CreateCompatibleDC(hDC);
  HBITMAP oldBitmap = (HBITMAP)SelectObject(hmemDC, spriteSheetMask);
  if (g_player.face == 0) {
    StretchBlt(hDC, g_player.x - PLAYER_SIZE / 2, g_player.y - PLAYER_SIZE / 2,
               PLAYER_SIZE, PLAYER_SIZE, hmemDC, x, y, width, height, SRCAND);
    oldBitmap = (HBITMAP)SelectObject(hmemDC, spriteSheet);
    StretchBlt(hDC, g_player.x - PLAYER_SIZE / 2, g_player.y - PLAYER_SIZE / 2,
               PLAYER_SIZE, PLAYER_SIZE, hmemDC, x, y, width, height, SRCPAINT);
    SelectObject(hmemDC, oldBitmap);
  } else if (g_player.face == 1) {
    StretchBlt(hDC, g_player.x + PLAYER_SIZE / 2, g_player.y - PLAYER_SIZE / 2,
               -PLAYER_SIZE, PLAYER_SIZE, hmemDC, x, y, width, height, SRCAND);
    oldBitmap = (HBITMAP)SelectObject(hmemDC, spriteSheet);
    StretchBlt(hDC, g_player.x + PLAYER_SIZE / 2, g_player.y - PLAYER_SIZE / 2,
               -PLAYER_SIZE, PLAYER_SIZE, hmemDC, x, y, width, height,
               SRCPAINT);
    SelectObject(hmemDC, oldBitmap);
  }
  DeleteDC(hmemDC);
}

// ������
void InitItems(int map[MAP_HEIGHT][MAP_WIDTH]) {
  for (int y = 0; y < MAP_HEIGHT; y++) {
    for (int x = 0; x < MAP_WIDTH; x++) {
      if (map[y][x] == 5) {
        GenerateItem(x, y, 0);
      }
    }
  }
}
void GenerateItem(int x, int y, int num) {
  Item newItem;
  newItem.x = x;
  newItem.y = y;
  newItem.disable = true;
  g_items.push_back(newItem);
}
void DrawItem(HDC hDC) {
  for (const auto& item : g_items) {
    if (!item.disable) {
      item_EnhanceJump.TransparentBlt(hDC, item.x * GRID, item.y * GRID, GRID,
                                      GRID, RGB(0, 255, 0));
    }
  }
}
void DeleteAllItems() { g_items.clear(); }
// ��
void InitEnemy(int map[MAP_HEIGHT][MAP_WIDTH]) {
  for (int y = 0; y < MAP_HEIGHT; y++) {
    for (int x = 0; x < MAP_WIDTH; x++) {
      if (map[y][x] == 4) {  // ��
        GenerateEnemy(x, y);
      }
    }
  }
}
void GenerateEnemy(int x, int y) {
  Enemy newEnemy;
  newEnemy.x = x;
  newEnemy.y = y;
  g_enemies.push_back(newEnemy);
}
void DrawEnemies(HDC hDC) {
  for (const auto& enemy : g_enemies) {
    cannon.StretchBlt(hDC, enemy.x * GRID, enemy.y * GRID, GRID, GRID, SRCCOPY);
  }
}
void DeleteAllEnemies() { g_enemies.clear(); }

void DrawBullets(HDC hdc) {
  HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
  SelectObject(hdc, hBrush);
  for (const auto& bullet : g_bullets) {
    if (bullet.x >= 0 && bullet.x <= BOARD_WIDTH && bullet.y >= 0 &&
        bullet.y <= BOARD_HEIGHT) {
      Ellipse(hdc, bullet.x - 20, bullet.y - 20, bullet.x + 20, bullet.y + 20);
    }
  }
  DeleteObject(hBrush);
}
void DeleteAllBullets() { g_bullets.clear(); }

// recv���� �����ͷ� ��� ���� ������Ʈ, ���� ���۷� ���޹��� ������ �ؼ�
void Update() {
  for (auto& item : g_items) {
    // item�� disable�� ���޹��� disable�� ������Ʈ
  }
    // �� ������ ���޹����� ��� ���� �ʱ�ȭ    ex) if(header == 2)
    /* if (map_num == 1)
      InitMap(map, map1);
    else if (map_num == 2)
      InitMap(map, map2);
    InitPlayer();
    DeleteAllEnemies();
    DeleteAllBullets();
    DeleteAllItems();
    InitEnemy(map);
    InitItems(map);*/
  
  if (map_num == 3) {
      // 4��° �� �Ǵ� ���� ���Ḧ ���޹����� ����� �ʿ��� ������ ������Ʈ
  }

    // ������� �÷��̾� ����� ���� ��������Ʈ ������Ʈ 
    // 1. dx, dy�� ������� ���������Ƿ� �ʿ��� �κ��� ���¸� �߰��� server Ÿ�̸ӽ����忡�� �÷��̾� ���¸� ������ ��, ��� ���¸� ���� �ʿ䰡 �����
    // 2. ��������Ʈ�� �� �÷��̾��� ����� �̽��ϰų� �� �÷��̾� ���� ��������Ʈ �� �� �����

  if (g_player.isWarking) {  // �ȱ� ��������Ʈ
    if ((spriteX += spriteWidth) > 230) {
      spriteX = 0;
      spriteY = 24;
      spriteHeight = 24;
    }
  }

  else if (g_player.isCharging) {
    spriteX = 0;
    spriteY = 116;
    spriteHeight = 22;
        /*if (g_player.jumpSpeed == -20) { Ǯ�����Ǹ� ��������Ʈ �ٲ�
          spriteX = 30;
        }*/
  } 

  else if (g_player.isFalling) {
    if ((spriteX += spriteWidth) > 119) {
      spriteX = 0;
    }
    spriteY = 48;
    spriteHeight = 29;
  } 

  else if (g_player.isJumping /*&& !g_player.isSliding*/) {
    if ((spriteX += spriteWidth) > 59) {
      spriteX = 0;
    }
    spriteY = 77;
    spriteHeight = 39;
  }

  else if (g_player.isSliding) {
    if ((spriteX += spriteWidth) > 29) {
      spriteX = 0;
    }
    spriteY = 138;
    spriteHeight = 25;
  }

  else {
    if ((spriteX += spriteWidth) > 230) {
      spriteX = 0;
    }
    spriteY = 0;
    spriteHeight = 24;
  }
}
    