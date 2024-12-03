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
#include "../Server/sendParam.hpp"

namespace server_mock
{
    int constexpr BUF_SIZE = 100;
    int constexpr SERVER_PORT = 9000;

    int send_player_info(SOCKET player_1_sock)
    {
        using HeaderType = sendParam::PKT_CAT;
        using InfoType = sendParam::sendParam;
        auto constexpr info_size = sizeof(HeaderType);
        auto constexpr header_size = sizeof(InfoType);
        auto constexpr packet_size = header_size + info_size;

        std::array<char, packet_size * 2> buf{};
        auto temp_player_1_info = InfoType{
            .x = 10,
            .y = 15,
            .acting = 1,
            .face = true,
        };

        auto temp_player_2_info = InfoType{
            .x = 3,
            .y = 10,
            .acting = 1,
            .face = false,
        };

        std::size_t total_packet_size = packet_size * 2;
        std::memcpy(buf.data(), &temp_player_1_info, packet_size);
        std::memcpy(std::next(buf.data(), packet_size), &temp_player_2_info,
                    packet_size);
        return send(player_1_sock, buf.data(), total_packet_size, 0);
    }

    void recv_handler(SOCKET client_sock)
    {
        int return_value{};
        int counted{100};
        while (true)
        {
            // 버퍼
            std::array<char, BUF_SIZE> buf{};
            return_value = recv(client_sock, buf.data(), 1, MSG_WAITALL);
            switch (return_value)
            {
            case SOCKET_ERROR:
                {
                    // 수신 문제
                    err_display(" : recv error");
                    return;
                }
            case 0:
                {
                    // 접속 종료 시 처리
                    err_display(" : disconnected");
                    return;
                }
            }
            if (++counted > 100)
            {
                std::print("\nrecved :");
                counted = 0;
            }
            std::print("{}", std::string_view{buf});
            int packet_size{};

            if (true)
            {
                return_value = send_player_info(client_sock);
            }
            else
            {
                auto temp_map_info = sendParam::PKT_CAT::CHANGE_MAP;
                packet_size = sizeof(temp_map_info);
                std::memcpy(buf.data(), &temp_map_info, packet_size);
                return_value = send(client_sock, buf.data(), packet_size, 0);
            }

            switch (return_value)
            {
            case SOCKET_ERROR:
                {
                    // 수신 문제
                    err_display(" : recv error");
                    return;
                }
            case 0:
                {
                    // 접속 종료 시 처리
                    err_display(" : disconnected");
                    return;
                }
            }
            std::print(", send {} {}", std::string_view{buf}, return_value);
        }
    }

} // namespace server_mock

int main()
{
    using namespace server_mock;
    // std::print("\x1B%G");
    WSADATA wsa{};
    // 윈속 초기화
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        std::exit(-1);
    };
    // 대기 소켓 등록
    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == listen_sock)
    {
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
    if (SOCKET_ERROR == return_value)
    {
        err_quit("bind()");
    }

    return_value = listen(listen_sock, SOMAXCONN);
    if (SOCKET_ERROR == return_value)
    {
        err_quit("listen()");
    }

    // 접속된 클라이언트용 소켓 주소
    SOCKET client_sock{};
    sockaddr_in client_addr{};
    int addr_len{};

    // accept()
    addr_len = sizeof(client_addr);
    client_sock = accept(listen_sock, (sockaddr *)&client_addr, &addr_len);
    if (INVALID_SOCKET == client_sock)
    {
        err_display("accept()");
    }
    else
    {
        recv_handler(client_sock);
        closesocket(listen_sock);
    }

    WSACleanup();
}
