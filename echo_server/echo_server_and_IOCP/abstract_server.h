#ifndef ABSTRACT_SERVER_H
#define ABSTRACT_SERVER_H

#include "../echo_server/sockets_wrapper.h"

DWORD WINAPI WorkingThread(LPVOID CompletionPortID);

void free_data(my_completion_key *received_key, additional_info *data);

class abstract_server {
	string addres_of_main_socket;
	
	int address_family;
	int type;
	int protocol;
	int port;
	int num_of_working_threads;
	
	size_t buffer_size;
	
	socket_descriptor main_socket;
	
	struct mess_to_working_thread {
		HANDLE iocp;
		abstract_server *server;
		
		mess_to_working_thread(HANDLE iocp, abstract_server *server) 
		: iocp(iocp), server(server) {
		}
	};
	
public:
	
	abstract_server(string addres_of_main_socket, int address_family, int type, int protocol, int port,
					int num_of_working_threads, size_t buffer_size = 1024) 
	: addres_of_main_socket(addres_of_main_socket), address_family(address_family), type(type), protocol(protocol),
	  port(port), num_of_working_threads(num_of_working_threads), buffer_size(buffer_size) {
		
		socket_descriptor main_socket = create_socket(address_family, type, protocol);
		bind_socket(main_socket, address_family, inet_addr(&addres_of_main_socket[0]), htons(port));
		
		this->main_socket = std::move(main_socket);
	}
	
	void start() {
		if (listen(main_socket.get_sd(), SOMAXCONN) == SOCKET_ERROR) {
			throw new socket_exception("listen function failed with error: " + to_str(WSAGetLastError()) + "\n");
		}
		
		IO_completion_port port(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, num_of_working_threads));
		if (port.get_handle() == NULL) {
			throw new socket_exception("Error in creation IO Completion Port : " + to_str(GetLastError()) + "\n");
		}
		
		for (int w = 0; w < num_of_working_threads; w++) {
			DWORD thread_id;
			HANDLE handle = CreateThread(NULL, 0, WorkingThread, new mess_to_working_thread(port.get_handle(), this), 0, &thread_id);
			if (handle == NULL) {
				throw new socket_exception("Error in creation new thread : " + to_str(GetLastError()) + "\n");
			}
			CloseHandle(handle);
		}
		
		while (1) {
			socket_descriptor client = accept_socket(main_socket);
			LOG("Connect to socket " << client.get_sd() << "\n");
			
			my_completion_key *key;
			
			unsigned int client_sd = client.get_sd();
			
			try {
				key = new my_completion_key(std::move(client));
			} catch (...) {
				LOG("Allocation error.");
				throw;
			}
			
			CreateIoCompletionPort((HANDLE)client_sd, port.get_handle(), (DWORD)key, 0);
			
			additional_info *info;
			try {
				info = new additional_info(buffer_size);
			} catch (...) {
				LOG("Allocation error.");
				throw;
			}
			
			DWORD flags = 0;
			DWORD unused;
			
			if (WSARecv(client_sd, &info->data_buff, (DWORD)1, &unused, &flags, &info->overlapped, NULL) == SOCKET_ERROR) {
				if (WSAGetLastError() != ERROR_IO_PENDING) {
					throw new socket_exception("Error in WSARecv : " + to_str(WSAGetLastError()) + "\n");
				}
			}
		}
	}
	friend DWORD WINAPI WorkingThread(LPVOID CompletionPortID);
	
protected:
	
	void recv(my_completion_key* received_key, additional_info* data) {
		data->clear();
		DWORD received_bytes;
		
		DWORD flags = 0;
		
		if (WSARecv(received_key->sd.get_sd(), &data->data_buff, 1, &received_bytes/*unused*/, &flags,
				&data->overlapped, NULL) == SOCKET_ERROR) {
						if ((WSAGetLastError() == 10054) || (WSAGetLastError() == 10053)) {
							free_data(received_key, data);
							return;
						}
						
						if (WSAGetLastError() != ERROR_IO_PENDING) {
							LOG("!!!\n");
							throw new socket_exception("Error in WSARecv : " + to_str(WSAGetLastError()) + "\n");
						}
		}
	}
	
	void send(my_completion_key* received_key, additional_info* data) {
		{
			LOG("Sending bytes : ");
			LOG(data->data_buff.len << "\n");
			
//				for (int w = 0; w < data->data_buff.len; w++) {
//					LOG(*(data->data_buff.buf + w));
//				}
//				LOG("\n");
		}
		DWORD received_bytes;
		
		if (WSASend(received_key->sd.get_sd(), &data->data_buff, 1, &received_bytes/*unused*/, 0, 
					&data->overlapped, NULL) == SOCKET_ERROR) {
						if (WSAGetLastError() == 10054) { // подключение разорвано
							LOG("In WSASend connection broken.\n");
							free_data(received_key, data);
							return;
						}
						
						if (WSAGetLastError() != ERROR_IO_PENDING) {
							throw new socket_exception("Error in WSASend : " + to_str(WSAGetLastError()) + "\n");
						}
		}
		
//			Sleep(2000);
		
	}
	
};


#endif // ABSTRACT_SERVER_H
