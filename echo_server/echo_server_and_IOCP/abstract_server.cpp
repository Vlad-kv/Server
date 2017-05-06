#include "abstract_server.h"
//
//void abstract_server::free_data(abstract_server::my_completion_key *received_key, abstract_server::server_buffer *data) {
//	delete received_key;
//	delete data;
//}
//
//struct abstract_server::my_OVERLAPED {
//	static const int NO_OPERATION_KEY = 0, RECV_KEY = 1, SEND_KEY = 2, ACCEPT_KEY = 3;
//private:
//	OVERLAPPED overlapped;
//	server_buffer* info;
//	int last_operation_type;
//	socket_descriptor client;
//	
//	friend abstract_server;
//public:	
//	int get_operation_type() {
//		return last_operation_type;
//	}
//	
//	my_OVERLAPED(server_buffer* info, int last_operation_type, socket_descriptor&& client)
//	: info(info), last_operation_type(last_operation_type), client(std::move(client)) {
//		SecureZeroMemory((PVOID) &overlapped, sizeof(OVERLAPPED));
//	}
//	
//	my_OVERLAPED(server_buffer* info, int last_operation_type)
//	: info(info), last_operation_type(last_operation_type) {
//		SecureZeroMemory((PVOID) &overlapped, sizeof(OVERLAPPED));
//	}
//};
//
//class abstract_server::my_completion_key {
//	socket_descriptor sd;
//public:
//	
//	void* data;
//	
//	// TODO
//	unsigned int get_sd() const {
//		return sd.get_sd();
//	}
//	
//	my_completion_key(socket_descriptor &&sd) 
//	: sd(std::move(sd)), data(create_add_data_to_request()) {
//	}
//	
//	~my_completion_key() {
//		release_add_data_to_connection(data);
//	}
//};
//
/////--------------------------------------------------------------------
//
//void abstract_server::start_working(const IO_completion_port& iocp) {
//	DWORD transmited_bytes;
//	
//	my_completion_key* received_key;
//	server_buffer* data;
//	my_OVERLAPED* overlaped;
//	
//	socket_descriptor client;
//	
//	while (1) {
//		int res = GetQueuedCompletionStatus(iocp.get_handle(), &transmited_bytes, (LPDWORD)&received_key, 
//										(LPOVERLAPPED*)&overlaped, 500);
//		if ((res == 0) && (GetLastError() == 258)) {
//			LOG("-");
//			continue;
//		}
//		LOG("\n");
//		
//		data = overlaped->info;
//		int type_of_operation = overlaped->get_operation_type();
//		client = std::move(overlaped->client);
//		delete overlaped;
//		
//		if (res == 0) {
//			if (GetLastError() == 64) {
//				LOG("connection interrupted\n");
////				LOG(received_key->get_sd() << "\n");
//				
//				free_data(received_key, data);
//				continue;
//			}
//			
//			LOG("Error in GetQueuedCompletionStatus : " << GetLastError() << "\n");
//			return;
//		}
//		
//		switch (type_of_operation) {
//		case my_OVERLAPED::RECV_KEY :
//			on_recv_completion(received_key, data, transmited_bytes);
//			break;
//		case my_OVERLAPED::SEND_KEY :
//			on_send_completion(received_key, data, transmited_bytes);
//			break;
//		case my_OVERLAPED::ACCEPT_KEY : {
////			my_completion_key* key = new my_completion_key(std::move(client));
////			CreateIoCompletionPort((HANDLE)key->get_sd(), iocp.get_handle(), (DWORD)key, 0);
////			
////			on_accept(key);
////			accept();
//			break;
//		}
//		default :
//			throw new socket_exception("Unknown operation.");
//		}
//	}
//}
//
//abstract_server::abstract_server(string addres_of_main_socket, int address_family, int type, int protocol, int port,
//				int num_of_working_threads) 
//: addres_of_main_socket(addres_of_main_socket), address_family(address_family), type(type), protocol(protocol),
//  port(port), num_of_working_threads(num_of_working_threads) {
//	
//	socket_descriptor main_socket = create_socket(address_family, type, protocol);
//	bind_socket(main_socket, address_family, inet_addr(&addres_of_main_socket[0]), htons(port));
//	
//	main_completion_key = new my_completion_key(std::move(main_socket));
//}
//
//void abstract_server::start() {
//	if (listen(main_completion_key->get_sd(), SOMAXCONN) == SOCKET_ERROR) {
//		throw new socket_exception("listen function failed with error: " + to_str(WSAGetLastError()) + "\n");
//	}
//	
//	IO_completion_port port(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, num_of_working_threads));
//	if (port.get_handle() == NULL) {
//		throw new socket_exception("Error in creation IO Completion Port : " + to_str(GetLastError()) + "\n");
//	}
//	
//	GUID GuidAcceptEx = WSAID_ACCEPTEX;
//	
//	DWORD unused;
//	int res = WSAIoctl(main_completion_key->get_sd(), SIO_GET_EXTENSION_FUNCTION_POINTER,
//						&GuidAcceptEx, sizeof(GuidAcceptEx),
//						&lpfnAcceptEx, sizeof(lpfnAcceptEx),
//						&unused, NULL, NULL);
//	if (res == SOCKET_ERROR) {
//		throw new socket_exception("WSAIoctl failed with error : " + to_str(WSAGetLastError()) + "\n");
//	}
//	
//	CreateIoCompletionPort((HANDLE)main_completion_key->get_sd(), port.get_handle(), (DWORD)main_completion_key, 0);
//	
//	accept();
//	start_working(port);
//}
//
//void abstract_server::accept() {
//	socket_descriptor client = create_socket(address_family, type, protocol);
//	
//	server_buffer* info = new server_buffer(((sizeof (sockaddr_in) + 16) * 2));
//	
//	my_OVERLAPED* overlapped = new my_OVERLAPED(0, my_OVERLAPED::ACCEPT_KEY, std::move(client));
//	DWORD unused;
//	int res = lpfnAcceptEx(main_completion_key->get_sd(), overlapped->client.get_sd(), info->buffer,
//							0, sizeof (sockaddr_in) + 16, sizeof (sockaddr_in) + 16,
//							&unused, &overlapped->overlapped);
//	if (res == FALSE) {
//		if (WSAGetLastError() != ERROR_IO_PENDING) {
//			LOG("lpfnAcceptEx failed with error : " + to_str(WSAGetLastError()) + "\n");
//			return;
//		}
//	}
//}
//
//void abstract_server::recv(my_completion_key* received_key, server_buffer* data) {
//	data->clear();
//	DWORD received_bytes;
//	DWORD flags = 0;
//	
//	my_OVERLAPED* overlapped = new my_OVERLAPED(data, my_OVERLAPED::RECV_KEY);
//	
//	if (WSARecv(received_key->get_sd(), &data->data_buff, 1, &received_bytes/*unused*/, &flags,
//			&overlapped->overlapped, NULL) == SOCKET_ERROR) {
//					if ((WSAGetLastError() == 10054) || (WSAGetLastError() == 10053)) {
//						delete overlapped;
//						free_data(received_key, data);
//						return;
//					}
//					if (WSAGetLastError() != ERROR_IO_PENDING) {
//						delete overlapped;
//						free_data(received_key, data);
//						
//						throw new socket_exception("Error in WSARecv : " + to_str(WSAGetLastError()) + "\n");
//					}
//	}
//}
//
//void abstract_server::send(my_completion_key* received_key, server_buffer* data) {
//	{
//		LOG("Sending bytes : ");
//		LOG(data->data_buff.len << "\n");
//		
////		for (int w = 0; w < data->data_buff.len; w++) {
////			LOG(*(data->data_buff.buf + w));
////		}
////		LOG("\n");
//	}
//	
//	DWORD received_bytes;
//	
//	my_OVERLAPED* overlapped = new my_OVERLAPED(data, my_OVERLAPED::SEND_KEY);
//	
//	if (WSASend(received_key->get_sd(), &data->data_buff, 1, &received_bytes/*unused*/, 0, 
//				&overlapped->overlapped, NULL) == SOCKET_ERROR) {
//					if (WSAGetLastError() == 10054) { // подключение разорвано
//						delete overlapped;
//						
//						LOG("In WSASend connection broken.\n");
//						free_data(received_key, data);
//						return;
//					}
//					
//					if (WSAGetLastError() != ERROR_IO_PENDING) {
//						delete overlapped;
//						free_data(received_key, data);
//						throw new socket_exception("Error in WSASend : " + to_str(WSAGetLastError()) + "\n");
//					}
//	}
////	Sleep(2000);
//}


