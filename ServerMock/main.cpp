//
// Created by sang hyeon, son
//
#include <array>
#include <iostream>
#include <print>
#include <string_view>
#include <syncstream>
#include <thread>

#include "../Client/network_util.hpp"
#include "../Client/protocol.hpp"

namespace server_mock {
int constexpr BUF_SIZE = 1;
int constexpr SERVER_PORT = 9000;

void recv_handler(SOCKET client_sock) {
  int return_value{};
  int counted{100};
  while (true) {
    // 버퍼
    std::array<char, BUF_SIZE> buf{};
    return_value = recv(client_sock, buf.data(), buf.size(), MSG_WAITALL);
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
    auto a = game_protocol::PlayerInfoPacket{.header{1},
                                             .info{.x = 10,
                                                   .y = 15,
                                                   .isCharging = true,
                                                   .isJumping = false,
                                                   .isSliding = true,
                                                   .damaged = false,
                                                   .face = true,
                                                   .isItemDisable = false,
                                                   .bulletX = 20,
                                                   .bulletY = 25}};
    std::memcpy(buf.data(), &a, sizeof(a));
    return_value = send(client_sock, buf.data(), sizeof(a), 0);
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
    std::println("{}", std::string_view{buf});
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