#include <WS2tcpip.h>
#include <atlImage.h>
#include <math.h>
#include <time.h>

//
#include <windows.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <syncstream>
#include <vector>

#include "../Server/sendParam.hpp"
#include "network_util.hpp"
#include "protocol.hpp"
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
const int GRAVITY = 1;  // 중력 상수

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

HANDLE g_hSendThread{};
HANDLE g_hRecvThread{};
UINT constexpr WM_NETWORK_INFORM = WM_USER + 1;

// 전역 변수
struct Player {
  int x, y;
  bool damaged;
  char acting;
  bool face;
  bool EnhancedJumpPower;
} g_player;
Player otherPlayer;

struct Item {
  int x, y;
  bool disable;
  int interval;
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

void DrawSnowBg(HDC hDC);
void DrawDesertBg(HDC hDC);
void DrawSnowTile(HDC hDC);
void DrawDesertTile(HDC hDC);
void DrawForestBg(HDC hDC);
void DrawForestTile(HDC hDC);
void InitMap(int dst[MAP_HEIGHT][MAP_WIDTH], int src[MAP_HEIGHT][MAP_WIDTH]);
void InitPlayer(Player& player);
void DrawSprite(HDC hDC, const Player& player, const int& x, const int& y,
                const int& width, const int& height);
void InitItems(int map[MAP_HEIGHT][MAP_WIDTH]);
void GenerateItem(int x, int y, int num);
void DrawItem(HDC hDC);
void DeleteAllItems();
void InitEnemy(int map[MAP_HEIGHT][MAP_WIDTH]);
void GenerateEnemy(int x, int y);
void DrawEnemies(HDC hDC);
void DeleteAllEnemies();
void DrawBullets(HDC hDC);
void DeleteAllBullets();
void CheckItemPlayerCollisions(vector<Item>& items, const Player& player);
void ShootBullet();
void MoveBullets();
void CheckPlayerBulletCollisions(vector<Bullet>& bullets, const Player& player);

// WinMain 함수
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpszCmdParam, int nCmdShow) {
  srand((unsigned int)time(NULL));

  // 윈속 초기화
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

  // 총알 저장크기 지정
  g_bullets.reserve(100);

  while (1) {
    if (PeekMessage(&Message, NULL, 0, 0, PM_REMOVE)) {
      if (Message.message == WM_QUIT) break;
    }
    // #1 마우스 관련된 메세지를 무시하는 첫번째 방법
    // if (Message.message == WM_MOUSEMOVE || Message.message == WM_LBUTTONDOWN
    // || Message.message == WM_RBUTTONDOWN) {
    //    // 마우스 메시지 무시
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

struct RecvClientParam {
  SOCKET recv_socket;
  HWND window_handle;
};
struct PlayerInfoMSG {
  sendParam::playerInfo my_player, other_player;
  std::vector<sendParam::Bullet> bullets;
};

DWORD WINAPI RecvClient(LPVOID lp_param) {
  auto [server_sock, window_handle] =
      *(reinterpret_cast<RecvClientParam*>(lp_param));

  delete reinterpret_cast<RecvClientParam*>(lp_param);

  int return_value{};
  int constexpr BUFF_SIZE = 500;
  std::array<char, BUFF_SIZE> recv_buff{};
  int recved_buffer_size{};

  while (true) {
    // 데이터 받기
    return_value =
        recv(server_sock, std::next(recv_buff.data(), recved_buffer_size),
             recv_buff.size() - recved_buffer_size, 0);
    switch (return_value) {
      case SOCKET_ERROR: {
        // 수신 문제
        err_display(" : recv error");
        return -1;
      }
      case 0: {
        // 접속 종료 시 처리
        err_display(" : disconnected");
        ::PostMessage(window_handle, WM_NETWORK_INFORM,
                      static_cast<std::int8_t>(100),  // 임시 코드
                      0);
        return -1;
      }
    }

    // 받은 데이터 크기 반영
    recved_buffer_size += return_value;

    for (bool finished = false; !finished;) {
      // 받은 데이터 처리
      using HeaderType = sendParam::PKT_CAT;
      auto header = reinterpret_cast<HeaderType*>(recv_buff.data());
      switch (static_cast<HeaderType>(*header)) {
        case sendParam::PKT_CAT::PLAYER_INFO: {
          using PacketType = sendParam::sendParam_alt;
          using InfoType = sendParam::playerInfo;

          int constexpr kInfoHeaderSize = sizeof(HeaderType);
          int constexpr kInfoPacketSize = sizeof(PacketType);

          auto* packet_ptr = reinterpret_cast<PacketType*>(recv_buff.data());

          std::int32_t bullets_size =
              (recved_buffer_size - sizeof(sendParam::sendParam_alt)) /
              sizeof(sendParam::Bullet);
          // 메세지로 보낼 정보 구조체 할당받고 받은 데이터 읽기
          auto player_infoes =
              new PlayerInfoMSG{.my_player = packet_ptr->myInfo,
                                .other_player = packet_ptr->otherInfo};

          if (bullets_size > 0) {
            sendParam::Bullet* bullet_init =
                reinterpret_cast<sendParam::Bullet*>(
                    std::next(recv_buff.data(), kInfoPacketSize));
            auto bullet_range =
                std::ranges::subrange{bullet_init, bullet_init + bullets_size};
            player_infoes->bullets.reserve(bullet_range.size());
            player_infoes->bullets.append_range(bullet_range);
          }

          // 데이터를 다 받았는지 확인 못하는 형태로 전송됨
          // if (kInfoPacketSize > recved_buffer_size) {
          //  unfinished = false;
          //  break;
          //}

#ifndef NDEBUG
          std::println(wow, "PLAYER_INFO recv  {} = my {} {} other {} {}",
                       recved_buffer_size, packet_ptr->myInfo.x,
                       packet_ptr->myInfo.y, packet_ptr->otherInfo.x,
                       packet_ptr->otherInfo.y);

          // std::print(wow, "bullet :");
          // for (auto& bullet : player_infoes->bullets) {
          //   std::print(wow, "{:03} {:03}\t", bullet.x, bullet.y);
          // }
          // std::println(wow);
          wow.emit();
#endif  // !NDEBUG

          // 윈도우로 보내기
          ::PostMessage(window_handle, WM_NETWORK_INFORM,
                        static_cast<std::int8_t>(HeaderType::PLAYER_INFO),
                        reinterpret_cast<LPARAM>(player_infoes));

          // 버퍼 처리 - 다 받은걸로 생각하고 처리하는 상황이라 할 필요 없음
          // std::memcpy(recv_buff.data(),
          //            std::next(recv_buff.data(), kInfoPacketSize * 2),
          //            recved_buffer_size - kInfoPacketSize * 2);
          recved_buffer_size -= return_value;

          finished = true;
          break;
        }
        case sendParam::PKT_CAT::CHANGE_MAP: {
          using InfoType = std::int32_t;
          using PacketType = sendParam::MapInfoPacket;

          int constexpr kInfoSize = sizeof(InfoType);
          int constexpr kInfoHeaderSize = sizeof(HeaderType);
          int constexpr kInfoPacketSize = sizeof(PacketType);
          // 데이터 다 받았는지 확인
          if (kInfoPacketSize > recved_buffer_size) {
            finished = true;
            break;
          }

          // 메세지로 보낼 정보 구조체 할당 받고 받은 데이터 읽기
          auto map_num_info =
              reinterpret_cast<PacketType*>(recv_buff.data())->info.mapNum;

          // 윈도우로 보내기
          ::PostMessage(
              window_handle, WM_NETWORK_INFORM,
              static_cast<std::int8_t>(sendParam::PKT_CAT::CHANGE_MAP),
              (LPARAM)map_num_info);

#ifndef NDEBUG
          std::println(wow, "MapInfo recv  {} = {}", return_value,
                       map_num_info);
          wow.emit();
#endif  // !NDEBUG

          // 버퍼 처리
          std::memcpy(recv_buff.data(),
                      std::next(recv_buff.data(), kInfoPacketSize),
                      recved_buffer_size - kInfoPacketSize);
          recved_buffer_size -= kInfoPacketSize;

          finished = true;
          break;
        }
        default: {
          err_display(" : recv buffer handle error");
          finished = true;
          break;
        }
      }
    }
  }
  return 0;
}

DWORD WINAPI SendClient(LPVOID lp_param)
{
    SOCKET server_sock = (SOCKET)lp_param;
    int return_value{};

    USHORT left_check_value{};
    USHORT right_check_value{};
    USHORT space_check_value{};

    bool is_pressed_left{false};
    bool is_pressed_right{false};
    bool is_pressed_space{false};
    bool was_pressed_space{false};

    std::vector<char> buffer{};
    buffer.reserve(3);

    while (true)
    {
        // �Է� ���� Ȯ��
        left_check_value = GetAsyncKeyState(VK_LEFT);
        right_check_value = GetAsyncKeyState(VK_RIGHT);
        space_check_value = GetAsyncKeyState(VK_SPACE);

        // 버퍼 정리
        buffer.clear();

        // 유효 입력 처리

        // 좌우 처리
        is_pressed_left = (left_check_value & 0x8000) != 0;
        is_pressed_right = (right_check_value & 0x8000) != 0;

        if (!(is_pressed_left && is_pressed_right))
        {
            if (is_pressed_left)
            {
                buffer.push_back('0');
            }
            if (is_pressed_right)
            {
                buffer.push_back('1');
            }
        }

        //
        is_pressed_space = (space_check_value & 0x8000) != 0;

        if (is_pressed_space && !was_pressed_space)
        {
            buffer.push_back(' ');
            was_pressed_space = true;

#ifndef NDEBUG
            std::println(wow, "send sp {} = {}:{} left {:0x} right {:0x} space {:0x}",
                         return_value, std::string_view{buffer}, buffer.size(),
                         left_check_value, right_check_value, space_check_value);
            wow.emit();
            wow.flush();
#endif  // NDEBUG
        }
        else if (!is_pressed_space && was_pressed_space)
        {
            buffer.push_back('\b');
            was_pressed_space = false;

#ifndef NDEBUG
            std::println(wow,
                         "send bsp {} = {}:{} left {:0x} right {:0x} space {:0x}",
                         return_value, std::string_view{buffer}, buffer.size(),
                         left_check_value, right_check_value, space_check_value);
            wow.emit();
            wow.flush();
#endif  // NDEBUG
        }

        // 눌린 것이 없으면 넘김
        if (buffer.empty())
        {
            std::this_thread::yield();
            continue;
        }

        // 전송 및 로그
        return_value = send(server_sock, buffer.data(), buffer.size(), 0);

        switch (return_value)
        {
        case SOCKET_ERROR:
        {
            err_quit("result of send : sock error");
            break;
        }
        case 0:
        {
            err_quit("result of send : server disconnect");
            break;
        }
        }

        // 입력 대기
        Sleep(1000 / 60);
    }
    return 0;
}

//--- CImage 관련 변수 선언
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
static int spriteX2 = 0;
static int spriteY2 = 0;
static int spriteWidth = 30;
static int spriteHeight = 0;
static int spriteHeight2 = 0;
void Update();
// 타이머 콜백
void CALLBACK TimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
  switch (idEvent) {
    case 2: {
        if ((GetAsyncKeyState('s') & 0x8000) ||
          (GetAsyncKeyState('S') & 0x8000)) {
            // 접속 시도
            int return_value{};
            SOCKET server_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (INVALID_SOCKET == server_sock) {
                err_quit("socket() 호출");
            }

            DWORD opt_val = 1;
        setsockopt(server_sock, IPPROTO_TCP, TCP_NODELAY, (char const*)&opt_val,
                       sizeof(opt_val));
            sockaddr_in serveraddr{.sin_family = AF_INET,
                               .sin_port = htons(game_protocol::g_server_port)};
            return_value =
                inet_pton(AF_INET, game_protocol::g_server_address.data(),
                          &serveraddr.sin_addr);
        if (SOCKET_ERROR == return_value) {
                err_quit("유효하지 않은 주소가 입력되었습니다.");
            }

            return_value =
            connect(server_sock, reinterpret_cast<sockaddr const*>(&serveraddr),
                        sizeof(serveraddr));
        if (return_value == SOCKET_ERROR) {
                err_quit("connect() 호출");
            }

            // 접속 성공시 ClientSend, CliendRecv 생성
            g_hSendThread =
                CreateThread(NULL, 0, SendClient, (LPVOID)server_sock, 0, NULL);

            auto* recvParam = new RecvClientParam{.recv_socket = server_sock,
                                                  .window_handle = hWnd};
        g_hRecvThread = CreateThread(NULL, 0, RecvClient, recvParam, 0, NULL);
            recvParam = nullptr;

            map_num = 1;
            InitPlayer(g_player);
            InitPlayer(otherPlayer);
            InitMap(map, map0);
            InitEnemy(map);
            InitItems(map);
            KillTimer(hWnd, 2);
            SetTimer(hWnd, 3, 1000 / 60, (TIMERPROC)TimerProc);
        }
        break;
    }
    case 3: {
        Update();
        // player1, 2와 아이템 충돌 확인

      for (auto& item : g_items) {
        if (item.interval <= 0)
          item.disable = false;
        else
          item.interval--;
      }

      CheckItemPlayerCollisions(g_items, g_player);
      CheckItemPlayerCollisions(g_items, otherPlayer);

      // 서버에서 충돌처리 된 대로 총알 출력하는 것으로 결정


        break;
    }
    }

    InvalidateRect(hWnd, NULL, FALSE);
}

// 메인 함수

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
                         LPARAM lParam) {
  PAINTSTRUCT ps;
  HDC hDC;
  HDC mDC;
  HBITMAP hBitmap;
  RECT rt;
  LPARAM static network_checked_deallocated{};
  static int playerFrameIndex = 0;

  switch (message) {
    case WM_CREATE: {
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

      SetTimer(hWnd, 2, 1000 / 60, (TIMERPROC)TimerProc);
      break;
    }
    case WM_CHAR: {
      switch (wParam) {
        case 'Q':
          // 다음 맵에서도 정상 출력되는지 테스트
          map_num = 2;
          InitMap(map, map1);
          InitPlayer(g_player);
          InitPlayer(otherPlayer);
          DeleteAllEnemies();
          DeleteAllBullets();
          DeleteAllItems();
          InitEnemy(map);
          InitItems(map);
          // 다음 맵에서도 정상 출력되는지 테스트
          break;
        case 'q':
            PostQuitMessage(0);
          break;
      }
      break;
    }
    case WM_PAINT: {
      hDC = BeginPaint(hWnd, &ps);
      GetClientRect(hWnd, &rt);
      mDC = CreateCompatibleDC(hDC);
      hBitmap = CreateCompatibleBitmap(hDC, BOARD_WIDTH, BOARD_HEIGHT);
      SelectObject(mDC, (HBITMAP)hBitmap);

      //--- 모든 그리기를 메모리 DC에한다.  ---> 바꾼 부분: CImage 변수는
      // 전역변수로 선언하여 함수의 인자로 보내지 않도록 한다.
      if (map_num == 0)
        startImage.Draw(mDC, 0, 0, GRID * 12, GRID * 15);
      else if (map_num == 1) {
        DrawSnowBg(mDC);
        DrawSnowTile(mDC);
        DrawEnemies(mDC);
        DrawBullets(mDC);
        DrawSprite(mDC, g_player, spriteX, spriteY, spriteWidth, spriteHeight);
        DrawSprite(mDC, otherPlayer, spriteX2, spriteY2, spriteWidth,
                   spriteHeight);  // 플레이어2 출력
        DrawItem(mDC);
      } else if (map_num == 2) {
        DrawDesertBg(mDC);
        DrawDesertTile(mDC);
        DrawEnemies(mDC);
        DrawBullets(mDC);
        DrawSprite(mDC, g_player, spriteX, spriteY, spriteWidth, spriteHeight);
        DrawSprite(mDC, otherPlayer, spriteX2, spriteY2, spriteWidth,
                   spriteHeight);
        DrawItem(mDC);
      } else if (map_num == 3) {
        DrawForestBg(mDC);
        DrawForestTile(mDC);
        DrawEnemies(mDC);
        DrawBullets(mDC);
        DrawSprite(mDC, g_player, spriteX, spriteY, spriteWidth, spriteHeight);
        DrawSprite(mDC, otherPlayer, spriteX2, spriteY2, spriteWidth,
                   spriteHeight);
        DrawItem(mDC);
      } else if (map_num == 4) {
        endImage.Draw(mDC, BOARD_WIDTH - GRID * 13, BOARD_HEIGHT - GRID * 17,
                      GRID * 12, GRID * 15);
      }

      // 메모리 DC에서 화면 DC로 그림을 복사
      // #1 맵 전체를 그리기
      // BitBlt(hDC, 0, 0, BOARD_WIDTH, BOARD_HEIGHT, mDC, 0, 0, SRCCOPY);
      // #2 플레이어 주변의 영역을 윈도우 전체로 확대
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
    case WM_NETWORK_INFORM: {
      using HeaderType = sendParam::PKT_CAT;
      std::int8_t curr_cat = LOBYTE(wParam);

      if (lParam == network_checked_deallocated) {
        break;
      } else {
        network_checked_deallocated = lParam;
      }

      // recv에서 보낸 정보 처리
      switch (static_cast<HeaderType>(curr_cat)) {
        case HeaderType::PLAYER_INFO: {
          PlayerInfoMSG* player_infoes =
              reinterpret_cast<PlayerInfoMSG*>(lParam);

          g_player.x = player_infoes->my_player.x;
          g_player.y = player_infoes->my_player.y;

          otherPlayer.x = player_infoes->other_player.x;
          otherPlayer.y = player_infoes->other_player.y;

          g_player.acting = player_infoes->my_player.acting;
          otherPlayer.acting = player_infoes->other_player.acting;

          g_player.face = player_infoes->my_player.face;
          otherPlayer.face = player_infoes->other_player.face;

          g_bullets.clear();
          g_bullets.reserve(player_infoes->bullets.size());

          std::ranges::transform(
              player_infoes->bullets, std::back_inserter(g_bullets),
              [](sendParam::Bullet const& a_bullet) {
                return Bullet{
                    .x = a_bullet.x, .y = a_bullet.y, .dx = 0, .dy = 0};
              });

#ifndef NDEBUG
          std::println(
              wow,
              "player window get my x,y : ({}, {}), other x, y : ({}, {}) ",
              g_player.x, g_player.y, otherPlayer.x, otherPlayer.y);
          wow.emit();
          wow.flush();
#endif  // !NDEBUG
          delete player_infoes;

          break;
        }
        case sendParam::PKT_CAT::CHANGE_MAP: {
          // 맵 변경
          int const recv_map_num = LOWORD(lParam);

#ifndef NDEBUG
          std::println(wow, "recv_map_num window get {} in map_num {}",
                       recv_map_num, map_num);
          wow.emit();
          wow.flush();
#endif  // !NDEBUG

          if (recv_map_num != map_num) {
            map_num = recv_map_num;
            switch (map_num) {
              case 1: {
                InitMap(map, map0);
                InitPlayer(g_player);
                InitPlayer(otherPlayer);
                DeleteAllEnemies();
                DeleteAllBullets();
                DeleteAllItems();
                InitEnemy(map);
                InitItems(map);
                break;
              }
              case 2: {
                InitMap(map, map1);
                InitPlayer(g_player);
                InitPlayer(otherPlayer);
                DeleteAllEnemies();
                DeleteAllBullets();
                DeleteAllItems();
                InitEnemy(map);
                InitItems(map);
                break;
              }
              case 3: {
                InitMap(map, map2);
                InitPlayer(g_player);
                InitPlayer(otherPlayer);
                DeleteAllEnemies();
                DeleteAllBullets();
                DeleteAllItems();
                InitEnemy(map);
                InitItems(map);
                break;
              }
              case 4: {
                break;
              }
            }
          }

          break;
        }
      }

      break;
    }
    case WM_DESTROY: {
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
      KillTimer(hWnd, 1);
      // KillTimer(hWnd, 2);
      PostQuitMessage(0);
      break;
    }
    default: {
      return DefWindowProc(hWnd, message, wParam, lParam);
    }
  }
  return 0;
}

// 키입력
bool spaceKeyReleased = true;

// 맵
void DrawSnowTile(HDC hDC) {
  // 칸당 96x96
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
  // 칸당 32x32
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
  // 칸당 64x64
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
// 플레이어
void InitPlayer(Player& player) {
  player.x = (MAP_WIDTH - 7) * GRID;
  player.y = (MAP_HEIGHT - 4) * GRID;
  player.acting = 0;
  player.damaged = false;
  player.face = 0;
}

void DrawSprite(HDC hDC, const Player& player, const int& x, const int& y,
                const int& width, const int& height) {
  HDC hmemDC = CreateCompatibleDC(hDC);
  HBITMAP oldBitmap = (HBITMAP)SelectObject(hmemDC, spriteSheetMask);
  if (player.face == 0) {
    StretchBlt(hDC, player.x - PLAYER_SIZE / 2, player.y - PLAYER_SIZE / 2,
               PLAYER_SIZE, PLAYER_SIZE, hmemDC, x, y, width, height, SRCAND);
    oldBitmap = (HBITMAP)SelectObject(hmemDC, spriteSheet);
    StretchBlt(hDC, player.x - PLAYER_SIZE / 2, player.y - PLAYER_SIZE / 2,
               PLAYER_SIZE, PLAYER_SIZE, hmemDC, x, y, width, height, SRCPAINT);
    SelectObject(hmemDC, oldBitmap);
  } else if (player.face == 1) {
    StretchBlt(hDC, player.x + PLAYER_SIZE / 2, player.y - PLAYER_SIZE / 2,
               -PLAYER_SIZE, PLAYER_SIZE, hmemDC, x, y, width, height, SRCAND);
    oldBitmap = (HBITMAP)SelectObject(hmemDC, spriteSheet);
    StretchBlt(hDC, player.x + PLAYER_SIZE / 2, player.y - PLAYER_SIZE / 2,
               -PLAYER_SIZE, PLAYER_SIZE, hmemDC, x, y, width, height,
               SRCPAINT);
    SelectObject(hmemDC, oldBitmap);
  }
  DeleteDC(hmemDC);
}

// 아이템
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
  newItem.disable = false;
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
// 적
void InitEnemy(int map[MAP_HEIGHT][MAP_WIDTH]) {
  for (int y = 0; y < MAP_HEIGHT; y++) {
    for (int x = 0; x < MAP_WIDTH; x++) {
      if (map[y][x] == 4) {  // 적
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

// recv받은 데이터로 출력 정보 업데이트, 수신 버퍼로 전달받은 데이터 해석
void Update() {
  for (auto& item : g_items) {
    // item의 disable을 전달받은 disable로 업데이트
  }

  if (map_num == 4 /*|| 게임 종료(연결 끊김)*/) {
    // 4번째 맵 또는 게임 종료를 전달받으면 종료시 필요한 정보로 업데이트
  }

  // 승패 확인하는 해석도 넣어야함

  // 여기부터 플레이어 출력을 위한 스프라이트 오프셋 업데이트
  // 1. dx, dy를 멤버에서 제거했으므로 필요한 부분은 상태를 추가함 server
  // 타이머스레드에서 플레이어 상태를 전송할 때, 모든 상태를 보낼 필요가
  // 없어보임
  // 2. 각 플레이어 전용 스프라이트 오프셋 두 개
  // 플레이어1의 스프라이트 오프셋 업데이트
  // acting 0: idle, 1: walking, 2: charging, 3: chargedfull, 4: jumping, 5:
  // falling, 6: sliding
  if (g_player.acting == '1') {  // 걷기 스프라이트
    if ((spriteX += spriteWidth) > 230) {
      spriteX = 0;
      spriteY = 24;
      spriteHeight = 24;
    }
  }

  else if (g_player.acting == '2') {
    spriteX = 0;
    spriteY = 116;
    spriteHeight = 22;

  } else if (g_player.acting == '3') {
    spriteX = 30;
    spriteY = 116;
    spriteHeight = 22;
  }

  else if (g_player.acting == '5') {
    if ((spriteX += spriteWidth) > 119) {
      spriteX = 0;
    }
    spriteY = 48;
    spriteHeight = 29;
  }

  else if (g_player.acting == '4' /*&& !g_player.isSliding*/) {
    if ((spriteX += spriteWidth) > 59) {
      spriteX = 0;
    }
    spriteY = 77;
    spriteHeight = 39;
  }

  else if (g_player.acting == '6') {
    if ((spriteX += spriteWidth) > 29) {
      spriteX = 0;
    }
    spriteY = 138;
    spriteHeight = 25;
  }

  else if (g_player.acting == '0') {
    if ((spriteX += spriteWidth) > 230) {
      spriteX = 0;
    }
    spriteY = 0;
    spriteHeight = 24;
  }

  // 플레이어2의 스프라이트 오프셋 업데이트
  if (otherPlayer.acting == '1') {  // 걷기 스프라이트
    if ((spriteX2 += spriteWidth) > 230) {
      spriteX2 = 0;
      spriteY2 = 24;
      spriteHeight2 = 24;
    }
  }

  else if (otherPlayer.acting == '2') {
    spriteX2 = 0;
    spriteY2 = 116;
    spriteHeight2 = 22;
  }

  else if (otherPlayer.acting == '3') {
    spriteX2 = 30;
    spriteY2 = 116;
    spriteHeight2 = 22;
  }

  else if (otherPlayer.acting == '5') {
    if ((spriteX2 += spriteWidth) > 119) {
      spriteX2 = 0;
    }
    spriteY2 = 48;
    spriteHeight2 = 29;
  }

  else if (otherPlayer.acting == '4' /*&& !g_player.isSliding*/) {
    if ((spriteX2 += spriteWidth) > 59) {
      spriteX2 = 0;
    }
    spriteY2 = 77;
    spriteHeight2 = 39;
  }

  else if (otherPlayer.acting == '6') {
    if ((spriteX2 += spriteWidth) > 29) {
      spriteX2 = 0;
    }
    spriteY2 = 138;
    spriteHeight2 = 25;
  }

  else if (otherPlayer.acting == '0') {
    if ((spriteX2 += spriteWidth) > 230) {
      spriteX2 = 0;
    }
    spriteY2 = 0;
    spriteHeight2 = 24;
  }
}

void CheckItemPlayerCollisions(vector<Item>& items, const Player& player) {
  for (auto it = items.begin(); it != items.end();) {
    if (player.x >= it->x * GRID && player.x <= (it->x + 1) * GRID &&
        player.y >= it->y * GRID && player.y <= (it->y + 1) * GRID) {
      it->disable = true;
      it->interval = 60;
    }
    ++it;
  }
}

void ShootBullet() {
  for (const auto& enemy : g_enemies) {
    Bullet newBullet;
    newBullet.x = (enemy.x + 1) * GRID;  // 적의 위치에서 총알이 나가도록 설정
    newBullet.y = enemy.y * GRID + GRID / 2;
    newBullet.dx = 2;
    newBullet.dy = 0;
    g_bullets.push_back(newBullet);
  }
}

void MoveBullets() {
  for (auto it = g_bullets.begin(); it != g_bullets.end();) {
    it->x += it->dx;
    it->y += it->dy;
    if (it->x < 0 || it->x > BOARD_WIDTH) {
      it = g_bullets.erase(it);
    } else {
      ++it;
    }
  }
}

void CheckPlayerBulletCollisions(vector<Bullet>& bullets,
                                 const Player& player) {
  for (auto it = bullets.begin(); it != bullets.end();) {
    if (it->x >= player.x - PLAYER_SIZE && it->x <= player.x + PLAYER_SIZE &&
        it->y >= player.y - PLAYER_SIZE && it->y <= player.y + PLAYER_SIZE) {
      it = bullets.erase(it);
    } else {
      ++it;
    }
  }
}