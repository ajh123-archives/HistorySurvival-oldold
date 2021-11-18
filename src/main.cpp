
#include <main.hpp>
#include <stdio.h>
#include <iostream>
#include <client_main.hpp>
#include <server_main.hpp>
#include <net_common.hpp>
#include <argparse/argparse.hpp>


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


int main(int argc, char* argv[])
{
    asio::error_code ec;
    asio::io_context context;
    asio::io_context::work idleWork(context);
    std::thread netThread = std::thread([&]() {context.run(); });
    argparse::ArgumentParser program("History Survival");

    if (game_type == GAME_TYPE_CLIENT){
        std::cout << "Running as client" << std::endl;
        return client_main();
    }
    if (game_type == GAME_TYPE_SERVER){
        program.add_argument("bind")
            .help("Address to bind the server to")
            .default_value(std::string("0.0.0.0"));

        try {
            program.parse_args(argc, argv);
        }
        catch (const std::runtime_error& err) {
            std::cerr << err.what() << std::endl;
            std::cerr << program;
            std::exit(1);
        }

        std::cout << "Running as server" << std::endl;
        return server_main(program.get<std::string>("bind"));
    }
    return -1;
}
