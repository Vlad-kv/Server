#include <iostream>

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

#include "logger.h"

#include "../echo_server/sockets_wrapper.h"

#include "abstract_server.h"

using namespace std;

const char* MAIN_SOCKET_ADDRES = "127.0.0.1";
const int DEFAULT_NUM_OF_WORKING_THREADS = 1;

const int DEFAULT_BUFFER_SIZE = 1024;

const int TIMEOUT = 1000;

//deque<pair<long long, *socket_descriptor>> time_queue;

int main(int argc, char* argv[]) {
    WSA_holder wsa_holder(MAKEWORD(2, 2));
    
    abstract_server server(MAIN_SOCKET_ADDRES, AF_INET, SOCK_STREAM, 0, 8001, 1);
    
    server.start();
    
    return 0;
}
