#pragma once

#ifndef SEND_PARAM
#define SEND_PARAM
#include <cstdint>
namespace sendParam {

#pragma pack(push, 1)
struct PacketHeader {
  std::int8_t header;
};

typedef struct Bullet {
  int x, y;
};

typedef struct playerInfo {
  int x, y;
  char acting;
  bool face;
};

typedef struct sendParam {
  PacketHeader header{.header = 1};
  playerInfo myInfo;
  playerInfo otherInfo;
};

struct MapInfo {
  std::int32_t mapNum;
};

struct MapInfoPacket {
  PacketHeader header{.header = 2};
  MapInfo info;
};

#pragma pack(pop)

enum class PKT_CAT : std::int8_t { PLAYER_INFO = 1, CHANGE_MAP = 2 };

}  // namespace sendParam

#endif