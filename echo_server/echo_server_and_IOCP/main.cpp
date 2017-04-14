#include <iostream>

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

#include "../echo_server/sockets_wrapper.h"
using namespace std;

const char* MAIN_SOCKET_ADDRES = "127.0.0.1";
const int DEFAULT_NUM_OF_WORKING_THREADS = 1;

const int DEFAULT_BUFFER_SIZE = 1024;

DWORD WINAPI WorkingThread(LPVOID CompletionPortID) {
	HANDLE iocp = (HANDLE) CompletionPortID;
	
	DWORD received_bytes;
	
	my_completion_key *received_key;
	additional_info *data;
	
	while (1) {
		if (GetQueuedCompletionStatus(iocp, &received_bytes, (LPDWORD)&received_key, 
										(LPOVERLAPPED *)&data, INFINITE) == 0) {
			cout << "Error in GetQueuedCompletionStatus : " << GetLastError() << "\n";
			return 0;
		}
		
		if (received_bytes == 0) {
			received_key->~my_completion_key();
			
			GlobalFree(received_key);
			additional_info::destroy(data);
			continue;
		}
		
		if (data->received_bytes == 0) {
			data->received_bytes = received_bytes;
		} else {
			data->sended_bytes += received_bytes;
		}
		
		if (data->sended_bytes < data->received_bytes) {
			data->data_buff.buf = data->buffer + data->sended_bytes;
			data->data_buff.len = data->received_bytes - data->sended_bytes;
			
			DWORD flags = 0;
			
			{
				cout << "Sending bytes : ";
				for (int w = 0; w < data->data_buff.len; w++) {
					cout << data->data_buff.buf + w;
				}
				cout << "\n";
			}
			
			if (WSASend(received_key->sd.get_sd(), &data->data_buff, 1, &received_bytes/*unused*/, flags, 
						&data->overlapped, NULL) == SOCKET_ERROR) {
							if (WSAGetLastError() != ERROR_IO_PENDING) {
								cout << "Error in WSASend : " << WSAGetLastError() << "\n";
								return 0;
							}
			}
		} else {
			data->clear();
			DWORD flags = 0;
			
			if (WSARecv(received_key->sd.get_sd(), &data->data_buff, 1, &received_bytes/*unused*/, &flags,
						&data->overlapped, NULL) == SOCKET_ERROR) {
							if (WSAGetLastError() != ERROR_IO_PENDING) {
								cout << "Error in WSARecv : " << WSAGetLastError() << "\n";
								return 0;
							}
			}
		}
	}
}

int main(int argc, char* argv[]) {
    WSA_holder wsa_holder(MAKEWORD(2, 2));
    
    socket_descriptor main_socket = create_socket(AF_INET, SOCK_STREAM, 0);
    bind_socket(main_socket, AF_INET, inet_addr(MAIN_SOCKET_ADDRES), htons(8001));
    
    if (listen(main_socket.get_sd(), SOMAXCONN) == SOCKET_ERROR) {
		cout << "listen function failed with error: " << WSAGetLastError() << "\n";
		return 1;
    }
    
    HANDLE port_h = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, DEFAULT_NUM_OF_WORKING_THREADS);
    
    IO_completion_port port(port_h);
    if (port.get_handle() == NULL) {
		cout << "Error in creation IO Completion Port : " << GetLastError() << "\n";
		return 0;
    }
    
    for (int w = 0; w < DEFAULT_NUM_OF_WORKING_THREADS; w++) {
		DWORD thread_id;
		HANDLE handle = CreateThread(NULL, 0, WorkingThread, port_h, 0, &thread_id);
		if (handle == NULL) {
			cout << "Error in creation new thread : " << GetLastError() << "\n";
			return 0;
		}
		CloseHandle(handle);
    }
    
    while (1) {
		socket_descriptor client = accept_socket(main_socket);
		cout << "Connect to socket " << client.get_sd() << "\n";
		
		my_completion_key *key = (my_completion_key*)GlobalAlloc(GPTR, sizeof(my_completion_key));
		if (key == NULL) {
			cout << "GlobalAlloc failed with error : " << GetLastError() << "\n";
			return 0;
		}
		unsigned int client_sd = client.get_sd();
		new(key)my_completion_key(std::move(client));
		
		CreateIoCompletionPort((HANDLE)client_sd, port.get_handle(), (DWORD)key, 0);
		
		additional_info *info = additional_info::create(DEFAULT_BUFFER_SIZE);
		
		DWORD flags = 0;
		DWORD unused;
		
		if (WSARecv(client_sd, &info->data_buff, (DWORD)1, &unused, &flags, &info->overlapped, NULL) == SOCKET_ERROR) {
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				cout << "Error in WSARecv : " << WSAGetLastError() << "\n";
				return 0;
			}
		}
    }
    
    return 0;
}
