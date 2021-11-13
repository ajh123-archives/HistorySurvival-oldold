
#include <main.hpp>
#include <stdio.h>
#include <client_main.hpp>

int main(void)
{
    if (game_type == GAME_TYPE_CLIENT){
        printf("Running as client\n");
        return client_main();
    }
    if (game_type == GAME_TYPE_SERVER){
        printf("Running as server\n");
    }
    return -1;
}
