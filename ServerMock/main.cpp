//
// Created by sang hyeon, son
//
#include <algorithm>
#include <array>
#include <iostream>
#include <print>
#include <string_view>
#include <syncstream>
#include <thread>
#include <vector>

#include "../Client/network_util.hpp"
#include "../Client/protocol.hpp"
#include "../Server/sendParam.hpp"

namespace server_mock {
int constexpr BUF_SIZE = 100;
int constexpr SERVER_PORT = 9000;

namespace server_types {
struct Bullet {
  int x, y;
};
}  // namespace server_types

std::vector server_local_bullets{server_types::Bullet{1, 4}, {2, 5}, {3, 4}};

int send_player_info(SOCKET player_1_sock) {
  using HeaderType = sendParam::PKT_CAT;
  using InfoType = sendParam::playerInfo;
  using PacketType = sendParam::sendParam;

  auto constexpr info_size = sizeof(InfoType);
  auto constexpr header_size = sizeof(HeaderType);

  std::array<char, BUF_SIZE> buf{};

  std::vector<sendParam::Bullet> send_bullets;
  send_bullets.reserve(server_local_bullets.size());

  // // 타입이 달라서 직접 변환이 안됨, 한쪽 타입에 맞추면 사용
  // std::ranges::copy(server_local_bullets, send_bullets.begin());

  std::ranges::transform(
      server_local_bullets, std::back_inserter(send_bullets),
      [](auto &pos) -> sendParam::Bullet { return {.x = pos.x, .y = pos.y}; });

  auto p1info = InfoType{
      .x = 10,
      .y = 15,
      .acting = 1,
      .face = true,
  };

  auto p2info = InfoType{
      .x = 3,
      .y = 10,
      .acting = 1,
      .face = false,
  };

  PacketType packet_smaple{.myInfo = p1info, .otherInfo = p2info};

  // 보낼 길이 계산용
  std::size_t packet_size{};

  // 고정길이 복사
  std::memcpy(buf.data(), &packet_smaple, sizeof(PacketType));
  packet_size += sizeof(PacketType);

  // 가변길이 복사
  auto bullets_byte_size =
      send_bullets.size() * sizeof(decltype(send_bullets)::value_type);
  std::memcpy(std::next(buf.data(), packet_size), send_bullets.data(),
              bullets_byte_size);
  packet_size += bullets_byte_size;

  return send(player_1_sock, buf.data(), packet_size, 0);
}

void recv_handler(SOCKET client_sock) {
  int return_value{};
  int counted{100};
  while (true) {
    // 버퍼
    std::array<char, BUF_SIZE> buf{};
    return_value = recv(client_sock, buf.data(), 1, MSG_WAITALL);
    switch (return_value) {
      case SOCKET_ERROR: {
        // 수신 문제
        err_display(" : recv error");
        return;
      }
      case 0: {
        // 접속 종료 시 처리
        err_display(" : disconnected");
        return;
      }
    }
    if (++counted > 100) {
      std::print("\nrecved :");
      counted = 0;
    }
    std::print("{}", std::string_view{buf});
    int packet_size{};

    if (true) {
      return_value = send_player_info(client_sock);
    } else {
      auto temp_map_info = sendParam::PKT_CAT::CHANGE_MAP;
      packet_size = sizeof(temp_map_info);
      std::memcpy(buf.data(), &temp_map_info, packet_size);
      return_value = send(client_sock, buf.data(), packet_size, 0);
    }

    switch (return_value) {
      case SOCKET_ERROR: {
        // 수신 문제
        err_display(" : recv error");
        return;
      }
      case 0: {
        // 접속 종료 시 처리
        err_display(" : disconnected");
        return;
      }
    }
    std::print(", send {} {}", std::string_view{buf}, return_value);
  }
}

}  // namespace server_mock

int main() {
  using namespace server_mock;
  // std::print("\x1B%G");
  WSADATA wsa{};
  // 윈속 초기화
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    std::exit(-1);
  };
  // 대기 소켓 등록
  SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (INVALID_SOCKET == listen_sock) {
    err_quit("socket()");
  }

  // 네이글 알고리즘 끄기
  DWORD opt_val = 1;
  setsockopt(listen_sock, IPPROTO_TCP, TCP_NODELAY, (char const *)&opt_val,
             sizeof(opt_val));

  // 소켓 주소 구조체 초기화, bind(), listen()
  sockaddr_in server_addr{
      .sin_family = AF_INET,
      .sin_port = htons(SERVER_PORT),
  };
  server_addr.sin_addr.s_addr = htonl(ADDR_ANY);

  int return_value =
      bind(listen_sock, (sockaddr *)&server_addr, sizeof(server_addr));
  if (SOCKET_ERROR == return_value) {
    err_quit("bind()");
  }

  return_value = listen(listen_sock, SOMAXCONN);
  if (SOCKET_ERROR == return_value) {
    err_quit("listen()");
  }

  // 접속된 클라이언트용 소켓 주소
  SOCKET client_sock{};
  sockaddr_in client_addr{};
  int addr_len{};

  // accept()
  addr_len = sizeof(client_addr);
  client_sock = accept(listen_sock, (sockaddr *)&client_addr, &addr_len);
  if (INVALID_SOCKET == client_sock) {
    err_display("accept()");
  } else {
    recv_handler(client_sock);
    closesocket(listen_sock);
  }

  WSACleanup();
}
