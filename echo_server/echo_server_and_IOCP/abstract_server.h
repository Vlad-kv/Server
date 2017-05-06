//#ifndef ABSTRACT_SERVER_H
//#define ABSTRACT_SERVER_H
//
//#include "../echo_server/sockets_wrapper.h"
//
//class abstract_server {
//	
//	struct my_OVERLAPED;
//public:
//	class my_completion_key;
//	
//	class server_buffer {
//	public:
//		WSABUF data_buff;
//		
//		char *buffer;
//		int buff_size;
//		
//		void* data;
//		
//		abstract_server* server;
//		
//		//	DWORD received_bytes; // long unsigned int
//		//	DWORD sended_bytes;
//		
//		server_buffer(int buff_size, abstract_server* server)
//		: buffer(new char[buff_size]), buff_size(buff_size), data(server->create_add_data_to_request()), server(server) {
//			clear();
//		}
//		
//		void clear() {
//	//		received_bytes = sended_bytes = 0;
//			
//			data_buff.len = buff_size;
//			data_buff.buf = buffer;
//		}
//		
//		~server_buffer() {
//			delete [] buffer;
//			server->release_add_data_to_request(data);
//		}
//	};
//private:
//	
//	string addres_of_main_socket;
//	
//	int address_family;
//	int type;
//	int protocol;
//	int port;
//	int num_of_working_threads;
//	LPFN_ACCEPTEX lpfnAcceptEx = NULL;
//	
//	my_completion_key *main_completion_key;
//	
//	void start_working(const IO_completion_port& iocp);
//	void accept();
//	
//public:
//	
//	abstract_server(string addres_of_main_socket, int address_family, int type, int protocol, int port, 
//					int num_of_working_threads);
//	
//	void start();
//protected:
//	
//	void free_data(my_completion_key *received_key, server_buffer *data);
//	
//	void recv(my_completion_key* received_key, server_buffer* data);
//	
//	void send(my_completion_key* received_key, server_buffer* data);
//	
//	virtual void* create_add_data_to_request() = 0;
//	virtual void release_add_data_to_request(void* data) = 0;
//	
//	virtual void* create_add_data_to_connection() = 0;
//	virtual void release_add_data_to_connection(void* data) = 0;
//	
////	virtual my_completion_key* create_completion_key(socket_descriptor&& client) {
////		try {
////			return new my_completion_key(std::move(client));
////		} catch (...) {
////			LOG("Allocation error.");
////			throw;
////		}
////	}
//	
////	virtual void* create_additional_info(size_t buffer_size) {
////		try {
////			return new server_buffer(buffer_size);
////		} catch (...) {
////			LOG("Allocation error.");
////			throw;
////		}
////	}
//	
//	virtual void on_accept(my_completion_key *key) = 0;
//	
//	virtual void on_send_completion(my_completion_key *key, server_buffer *info, DWORD transmited_bytes) = 0;
//	
//	virtual void on_recv_completion(my_completion_key *key, server_buffer *info, DWORD transmited_bytes) = 0;
//	
//};
//
//#endif // ABSTRACT_SERVER_H
