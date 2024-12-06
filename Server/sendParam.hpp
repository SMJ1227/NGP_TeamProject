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

// 다시 바꿔서 생각한 결과
typedef struct sendParam_alt {
  PacketHeader header{.header = 1};
  playerInfo myInfo;
  playerInfo otherInfo;
  // // 보낼 때는 버퍼에서 sendParam 위치 뒤에 복사하고
  // Bullet* bullets;
};

typedef struct recvParam_alt {
  PacketHeader header{.header = 1};
  playerInfo myInfo;
  playerInfo otherInfo;
  Bullet* bullets; // 읽을 때는 복사된 대로 읽기
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