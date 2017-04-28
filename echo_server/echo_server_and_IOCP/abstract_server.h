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
	LPFN_ACCEPTEX lpfnAcceptEx = NULL;
	
	my_completion_key *main_completion_key;
	
	struct mess_to_working_thread {
		HANDLE iocp;
		abstract_server *server;
		
		mess_to_working_thread(HANDLE iocp, abstract_server *server) 
		: iocp(iocp), server(server) {
		}
	};
	
	void start_working(const IO_completion_port& iocp);
	
public:
	
	abstract_server(string addres_of_main_socket, int address_family, int type, int protocol, int port,
					int num_of_working_threads) 
	: addres_of_main_socket(addres_of_main_socket), address_family(address_family), type(type), protocol(protocol),
	  port(port), num_of_working_threads(num_of_working_threads) {
		
		socket_descriptor main_socket = create_socket(address_family, type, protocol);
		bind_socket(main_socket, address_family, inet_addr(&addres_of_main_socket[0]), htons(port));
		
		main_completion_key = new my_completion_key(std::move(main_socket));
	}
	
	void start() {
		if (listen(main_completion_key->get_sd(), SOMAXCONN) == SOCKET_ERROR) {
			throw new socket_exception("listen function failed with error: " + to_str(WSAGetLastError()) + "\n");
		}
		
		IO_completion_port port(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, num_of_working_threads));
		if (port.get_handle() == NULL) {
			throw new socket_exception("Error in creation IO Completion Port : " + to_str(GetLastError()) + "\n");
		}
		
		GUID GuidAcceptEx = WSAID_ACCEPTEX;
		
		DWORD unused;
		int res = WSAIoctl(main_completion_key->get_sd(), SIO_GET_EXTENSION_FUNCTION_POINTER,
							&GuidAcceptEx, sizeof(GuidAcceptEx),
							&lpfnAcceptEx, sizeof(lpfnAcceptEx),
							&unused, NULL, NULL);
		if (res == SOCKET_ERROR) {
			throw new socket_exception("WSAIoctl failed with error : " + to_str(WSAGetLastError()) + "\n");
		}
		
		CreateIoCompletionPort((HANDLE)main_completion_key->get_sd(), port.get_handle(), (DWORD)main_completion_key, 0);
		
		accept();
		start_working(port);
	}
	friend DWORD WINAPI WorkingThread(LPVOID CompletionPortID);
	
	void accept() {
		socket_descriptor client = create_socket(address_family, type, protocol);
		
		additional_info* info = new additional_info(((sizeof (sockaddr_in) + 16) * 2));
		
		my_OVERLAPED* overlapped = new my_OVERLAPED(0, my_OVERLAPED::ACCEPT_KEY, std::move(client));
		DWORD unused;
		int res = lpfnAcceptEx(main_completion_key->get_sd(), overlapped->client.get_sd(), info->buffer,
								0, sizeof (sockaddr_in) + 16, sizeof (sockaddr_in) + 16,
								&unused, &overlapped->overlapped);
		if (res == FALSE) {
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				LOG("lpfnAcceptEx failed with error : " + to_str(WSAGetLastError()) + "\n");
				return;
			}
		}
	}
	
protected:
	
	void recv(my_completion_key* received_key, additional_info* data) {
		data->clear();
		DWORD received_bytes;
		DWORD flags = 0;
		
		my_OVERLAPED* overlapped = new my_OVERLAPED(data, my_OVERLAPED::RECV_KEY);
		
		if (WSARecv(received_key->get_sd(), &data->data_buff, 1, &received_bytes/*unused*/, &flags,
				&overlapped->overlapped, NULL) == SOCKET_ERROR) {
						if ((WSAGetLastError() == 10054) || (WSAGetLastError() == 10053)) {
							delete overlapped;
							free_data(received_key, data);
							return;
						}
						if (WSAGetLastError() != ERROR_IO_PENDING) {
							delete overlapped;
							free_data(received_key, data);
							
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
		
		my_OVERLAPED* overlapped = new my_OVERLAPED(data, my_OVERLAPED::SEND_KEY);
		
		if (WSASend(received_key->get_sd(), &data->data_buff, 1, &received_bytes/*unused*/, 0, 
					&overlapped->overlapped, NULL) == SOCKET_ERROR) {
						if (WSAGetLastError() == 10054) { // подключение разорвано
							delete overlapped;
							
							LOG("In WSASend connection broken.\n");
							free_data(received_key, data);
							return;
						}
						
						if (WSAGetLastError() != ERROR_IO_PENDING) {
							delete overlapped;
							free_data(received_key, data);
							throw new socket_exception("Error in WSASend : " + to_str(WSAGetLastError()) + "\n");
						}
		}
//			Sleep(2000);
	}
	
	virtual my_completion_key* create_completion_key(socket_descriptor&& client) {
		try {
			return new my_completion_key(std::move(client));
		} catch (...) {
			LOG("Allocation error.");
			throw;
		}
	}
	
	virtual additional_info* create_additional_info(size_t buffer_size) {
		try {
			return new additional_info(buffer_size);
		} catch (...) {
			LOG("Allocation error.");
			throw;
		}
	}
	
	virtual void on_accept(my_completion_key *key) {
		recv(key, create_additional_info(1024));
	}
	
	virtual void on_send_completion(my_completion_key *key, additional_info *info, DWORD transmited_bytes) {
		if (transmited_bytes == 0) {
			free_data(key, info);
			return;
		}
		info->sended_bytes += transmited_bytes;
		info->data_buff.buf += transmited_bytes;
		info->data_buff.len -= transmited_bytes;
		
		if (info->sended_bytes < info->received_bytes) {
			send(key, info);
		} else {
			recv(key, info);
		}
	}
	
	virtual void on_recv_completion(my_completion_key *key, additional_info *info, DWORD transmited_bytes) {
		if (transmited_bytes == 0) {
			free_data(key, info);
			return;
		}
		info->received_bytes = transmited_bytes;
		info->data_buff.len = transmited_bytes;
		send(key, info);
	}
	
};

#endif // ABSTRACT_SERVER_H
