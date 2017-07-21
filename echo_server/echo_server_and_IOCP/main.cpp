#include <iostream>

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

#include "logger.h"
#include "../echo_server/sockets_wrapper.h"
#include "echo_server.h"
#include "socket.h"

#include "timers.h"

using namespace std;

const char* MAIN_SOCKET_ADDRES = "127.0.0.1";
const int DEFAULT_BUFFER_SIZE = 1024;

int main(int argc, char* argv[]) {
    WSA_holder wsa_holder(MAKEWORD(2, 2));
    
    timer t_1(chrono::milliseconds(2000), [](){cout << "in t_1\n";});
    
    timer_thread t_t;
    
    cout << "After constructor\n";
    
    t_t.registrate_timer(t_1);
    
    cout << "After registration\n";
    Sleep(1000000000);
    
//    echo_server server(MAIN_SOCKET_ADDRES, AF_INET, SOCK_STREAM, 0, 8001);
//    
//    server.start();
//    Sleep(1<<30);
    return 0;
}
