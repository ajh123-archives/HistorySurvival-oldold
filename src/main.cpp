
#include <main.hpp>
#include <stdio.h>
#include <iostream>
#include <client_main.hpp>
#include <server_main.hpp>
#include <net_common.hpp>


std::vector<char> buffer(20 * 1024);
void getData(asio::ip::tcp::socket& socket) {
    socket.async_read_some(asio::buffer(buffer.data(), buffer.size()),
        [&](std::error_code ec, std::size_t length) {
            if (!ec) {
                std::cout << "\n\nRead " << length << " bytes" << std::endl;
                for (int i = 0; i < length; i++) {
                    std::cout << buffer[i];
                }

                getData(socket);
            }
        }
    );
}


int main(void)
{
    asio::error_code ec;
    asio::io_context context;
    asio::io_context::work idleWork(context);
    std::thread netThread = std::thread([&]() {context.run(); });

    if (game_type == GAME_TYPE_CLIENT){
        asio::io_service io_service;
        asio::ip::tcp::resolver resolver(io_service);
        asio::ip::tcp::resolver::query query("example.com", "80");
        asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);

        asio::ip::tcp::endpoint endpoint = iter->endpoint();
        asio::ip::tcp::socket socket(context);
        socket.connect(endpoint, ec);

        if (!ec) {
            std::cout << "Connected" << std::endl;
            if (socket.is_open()) {
                getData(socket);
                std::string request =
                    "GET /index.html HTTP/1.1\r\n"
                    "Host: example.com\r\n"
                    "Connection: close\r\n\r\n";
                socket.write_some(asio::buffer(request.data(), request.size()), ec);
            }
        }
        else {
            std::cout << "Failed to connect" << ec.message() << std::endl;
        }

        std::cout << "Running as client" << std::endl;
        return client_main();
    }
    if (game_type == GAME_TYPE_SERVER){
        std::cout << "Running as server" << std::endl;
        return server_main();
    }
    return -1;
}
