#ifndef MAP_H
#define MAP_H

// Window and Grid Constants
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

// Map Data
extern int map_num;
extern int tile_num;

extern int map0[MAP_HEIGHT][MAP_WIDTH];
extern int tile0[MAP_HEIGHT][MAP_WIDTH];

extern int map1[MAP_HEIGHT][MAP_WIDTH];
extern int tile1[MAP_HEIGHT][MAP_WIDTH];

extern int map2[MAP_HEIGHT][MAP_WIDTH];
extern int tile2[MAP_HEIGHT][MAP_WIDTH];

#endif  // MAP_H
