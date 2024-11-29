#pragma once

#ifndef GAME_PROTOCOL
#define GAME_PROTOCOL
#include <cstdint>
#include <string_view>
namespace game_protocol {

std::string_view const g_server_address{"127.0.0.1"};
int const g_server_port = 9000;

// PACKET_CATEGORY
enum class PKT_CAT : std::int8_t { PLAYER_INFO = 1, CHANGE_MAP = 2 };

// #pragma pack(push, 1)
// struct PacketHeader {
//   std::int8_t header;
// };
//
// struct PlayerInfo {
//   std::int32_t x, y;
//   bool isCharging;
//   bool isJumping;
//   bool isSliding;
//   bool damaged;
//   bool face;
//   bool isItemDisable;
//   std::int32_t bulletX, bulletY;
// };
//
// struct PlayerInfoPacket {
//   PacketHeader header{.header = 1};
//   PlayerInfo info;
// };
//
// struct MapInfo {
//   std::int32_t mapNum;
// };
//
// struct MapInfoPacket {
//   PacketHeader header{.header = 2};
//   MapInfo info;
// };
//
// #pragma pack(pop)

}  // namespace game_protocol

#endif