#pragma once

#ifndef SEND_PARAM
#define SEND_PARAM
#include <cstdint>
namespace sendParam {

#pragma pack(push, 1)
typedef struct sendParam {
  int x, y;
  char acting;
  bool face;
};
#pragma pack(pop)

enum class PKT_CAT : std::int8_t { PLAYER_INFO = 1, CHANGE_MAP = 2 };

}  // namespace sendParam

#endif