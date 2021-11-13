
#include <main.hpp>
#include <stdio.h>
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
    //https://github.com/ajh123-development/HistorySurvival-Data/raw/main/info.json
    if (game_type == GAME_TYPE_CLIENT){
        printf("Running as client\n");
        return client_main();
    }
    if (game_type == GAME_TYPE_SERVER){
        printf("Running as server\n");
    }
    return -1;
}
