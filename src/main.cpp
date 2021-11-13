
#include <main.hpp>
#include <stdio.h>
#include <iostream>
#include <client_main.hpp>

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif
#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>


int main(void)
{
    asio::error_code ec;
    asio::io_context context;

    //https://github.com/ajh123-development/HistorySurvival-Data/raw/main/info.json
    if (game_type == GAME_TYPE_CLIENT){
        asio::io_service io_service;
        asio::ip::tcp::resolver resolver(io_service);
        asio::ip::tcp::resolver::query query("github.com", "80");
        asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);

        asio::ip::tcp::endpoint endpoint = iter->endpoint();
        asio::ip::tcp::socket socket(context);
        socket.connect(endpoint, ec);

        if (!ec) {
            std::cout << "Connected" << std::endl;
        }
        else {
            std::cout << "Failed to connect" << ec.message() << std::endl;
        }

        printf("Running as client\n");
        return client_main();
    }
    if (game_type == GAME_TYPE_SERVER){
        printf("Running as server\n");
    }
    return -1;
}
