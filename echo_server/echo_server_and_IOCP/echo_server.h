//#ifndef ECHO_SERVER_H
//#define ECHO_SERVER_H
//
//#include "abstract_server.h"
//
//struct echo_data_to_request {
//};
//
//struct echo_data_to_connection {
//};
//
//class echo_server : abstract_server {
//
//public:
//	echo_server(string addres_of_main_socket, int address_family, int type, int protocol, int port, 
//					int num_of_working_threads)
//	: abstract_server(addres_of_main_socket, address_family, type, protocol, port, 
//						num_of_working_threads) {
//	}
//	
//	void on_accept(abstract_server::my_completion_key *key) {
//		recv(key, new abstract_server::server_buffer(1024));
//	}
//	
//	void on_send_completion(my_completion_key *key, server_buffer *info, DWORD transmited_bytes) {
//		if (transmited_bytes == 0) {
//			free_data(key, info);
//			return;
//		}
////		info->sended_bytes += transmited_bytes;
////		info->data_buff.buf += transmited_bytes;
////		info->data_buff.len -= transmited_bytes;
////		
////		if (info->sended_bytes < info->received_bytes) {
////			send(key, info);
////		} else {
////			recv(key, info);
////		}
//	}
//	
//	void on_recv_completion(my_completion_key *key, server_buffer *info, DWORD transmited_bytes) {
//		if (transmited_bytes == 0) {
//			free_data(key, info);
//			return;
//		}
////		info->received_bytes = transmited_bytes;
////		info->data_buff.len = transmited_bytes;
////		send(key, info);
//	}
//	
//	
//};
//
//#endif // ECHO_SERVER_H
