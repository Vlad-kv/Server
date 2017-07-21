#include "IO_completion_port.h"
#include <thread>

IO_completion_port::completion_key_decrementer::completion_key_decrementer(completion_key* key) 
: key(key) {
}
IO_completion_port::completion_key_decrementer::~completion_key_decrementer() {
	key->num_referenses--;
	if (key->num_referenses == 0) {
		delete key;
	}
}

///---------------------------

IO_completion_port::IO_completion_port(HANDLE handle)
: iocp_handle(handle) {
}

IO_completion_port::IO_completion_port(IO_completion_port &&port)
: iocp_handle(port.iocp_handle) {
	port.iocp_handle = INVALID_HANDLE_VALUE;
}

IO_completion_port::IO_completion_port()
: iocp_handle(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0)) {
	if (iocp_handle == NULL) {
		throw new socket_exception("CreateIoCompletionPort failed with error : " + std::to_string(GetLastError()) + "\n");
	}
}

HANDLE IO_completion_port::get_handle() const {
	return iocp_handle;
}

void IO_completion_port::close() {
	if (iocp_handle != INVALID_HANDLE_VALUE) {
		LOG("~~~~~~~~~~~~~\n");
		BOOL res = CloseHandle(iocp_handle);
		if (res == 0) {
			throw new socket_exception("Error in closing completion port : " + std::to_string(GetLastError()));
		}
	}
}

void IO_completion_port::registrate_socket(server_socket& sock) {
	completion_key *key = new completion_key(&sock);
	HANDLE res = CreateIoCompletionPort((HANDLE)sock.sd.get_sd(), iocp_handle, (DWORD)key, 0);
	if (res == NULL) {
		delete key;
		throw new socket_exception("CreateIoCompletionPort failed with error : " + std::to_string(GetLastError()) + "\n");
	}
	sock.key = key;
	key->num_referenses++;
}
void IO_completion_port::registrate_socket(client_socket& sock) {
	completion_key *key = new completion_key(&sock);
	HANDLE res = CreateIoCompletionPort((HANDLE)sock.sd.get_sd(), iocp_handle, (DWORD)key, 0);
	if (res == NULL) {
		delete key;
		throw new socket_exception("CreateIoCompletionPort failed with error : " + std::to_string(GetLastError()) + "\n");
	}
	sock.key = key;
	key->num_referenses++;
}

void IO_completion_port::run_in_this_thread() {
	DWORD transmited_bytes;
	
	completion_key* received_key;
	OVERLAPPED* overlapped;
	
	socket_descriptor client;
	
	while (1) {
		int res = GetQueuedCompletionStatus(iocp_handle, &transmited_bytes, (LPDWORD)&received_key, 
										(LPOVERLAPPED*)&overlapped, 500);
		if ((res == 0) && (GetLastError() == 258)) {
			LOG("-");
			continue;
		}
		LOG("\n");
		
		completion_key_decrementer decrementer(received_key);
		
		if (received_key->ptr == nullptr) {
			continue;
		}
		
		switch (received_key->type) {
			case completion_key::SERVER_SOCKET_KEY : {
				server_socket::server_socket_overlapped *real_overlaped = 
												(server_socket::server_socket_overlapped *)overlapped;
				
				socket_descriptor client = std::move(real_overlaped->sd);
				delete real_overlaped;
				
				if (res == 0) {
					if (received_key->ptr == nullptr) {
						LOG("nullptr on server completion_key.");
					} else {
						LOG("GetQueuedCompletionStatus failed with error : " 
														+ std::to_string(GetLastError()) + "\n");
					}
					break;
				}
				((server_socket*)received_key->ptr)->on_accept(std::move(client));
				break;
			}
			case completion_key::CLIENT_SOCKET_KEY : {
				client_socket::client_socket_overlapped *real_overlaped = 
										(client_socket::client_socket_overlapped*)overlapped;
				int type = real_overlaped->type_of_operation;
				delete real_overlaped;
				
				client_socket *real_ptr = (client_socket*)received_key->ptr;
				
				if (res == 0) {
					if (GetLastError() == 64) {
						LOG("connection interrupted\n");
						real_ptr->execute_on_disconnect();
						continue;
					}
					
					if (real_ptr == nullptr) {
						LOG("nullptr on client completion_key.");
						continue;
					}
					
					throw new socket_exception("Error in GetQueuedCompletionStatus : " +
												std::to_string(GetLastError()) + "\n");
				}
				
				if (type == client_socket::client_socket_overlapped::RECV_KEY) {
					real_ptr->on_read_completion(transmited_bytes);
					break;
				}
				if (type == client_socket::client_socket_overlapped::SEND_KEY) {
					real_ptr->on_write_completion(transmited_bytes);
					break;
				}
				throw new socket_exception("Uncnown operation code : " + std::to_string(type) + "\n");
			}
		}
	}
}

void call_run_in_this_thread(IO_completion_port *port) {
	port->run_in_this_thread();
}

std::unique_ptr<std::thread> IO_completion_port::run_in_new_thread() {
	std::unique_ptr<std::thread> new_thread(new std::thread(call_run_in_this_thread, this));
	return new_thread;
}

IO_completion_port::~IO_completion_port() {
	try {
		close();
	} catch (const socket_exception *ex) {
		LOG("Exception in destructor of IO_completion_port");
	}
}
