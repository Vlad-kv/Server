#include "IO_completion_port.h"
#include <thread>
#include <cassert>
#include <signal.h>

using namespace std;

bool operator<(const timer_holder& t_1, const timer_holder& t_2) {
	return (*t_1.t < *t_2.t);
}

timer_holder::timer_holder(timer *t)
: t(t) {
}

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

atomic_bool IO_completion_port::is_interrapted(false);

void IO_completion_port::signal_handler(int signal) {
	is_interrapted.store(true);
}

int IO_completion_port::get_time_to_wait() {
	if (timers.size() == 0) {
		return MAX_TIME_TO_WAIT;
	}
	int res = chrono::duration_cast<chrono::milliseconds>((*timers.begin()).t->expiration - chrono::steady_clock::now()).count();
	res = max(res, 0);
	res = min(res, MAX_TIME_TO_WAIT);
	return res;
}

void IO_completion_port::termination() {
	while (timers.size() > 0) {
		(*timers.begin()).t->unregistrate();
	}
	
	for (func_t &f : on_interrupt_f) {
		try {
			f();
		} catch (...) {
			LOG("Exception in on_interruption_event\n");
		}
	}
}

IO_completion_port::IO_completion_port()
: iocp_handle(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0)) {
	if (iocp_handle == NULL) {
		throw new socket_exception("CreateIoCompletionPort failed with error : " + to_string(GetLastError()) + "\n");
	}
	is_already_started.clear();
	to_notify = socket_descriptor(AF_INET, SOCK_STREAM, 0);
	
	server_socket temp_s(AF_INET, SOCK_STREAM, 0, nullptr);
	registrate_socket(temp_s);
	
	temp_s.bind_and_listen(AF_INET, "127.0.0.1", 8002); /// TODO - запросить у системы случайный
	temp_s.accept(AF_INET, SOCK_STREAM, 0);
	
	to_notify.connect(AF_INET, "127.0.0.1", 8002);
	
	DWORD transmited_bytes;
	completion_key* received_key;
	OVERLAPPED* overlapped;
	
	int res = GetQueuedCompletionStatus(iocp_handle, &transmited_bytes, (LPDWORD)&received_key,
										(LPOVERLAPPED*)&overlapped, INFINITE);
	if (res == FALSE) {
		throw new socket_exception("GetQueuedCompletionStatus failed with error "
										+ to_string(GetLastError()) + "\n");
	}
	assert(received_key->type == completion_key::SERVER_SOCKET_KEY);
	
	completion_key_decrementer decrementer(received_key);
	
	server_socket::server_socket_overlapped *real_overlaped = 
												(server_socket::server_socket_overlapped *)overlapped;
	notification_socket = client_socket(move(real_overlaped->sd));
	delete real_overlaped;
	
	notification_socket.set_on_read_completion(
		[this](int size) {
			if (size == 0) {
				
				/// TODO
			} else {
				LOG("Notification completed.\n");
				notification_socket.read_some(buff_to_get_notification, 1);
			}
		}
	);
	registrate_socket(notification_socket);
	notification_socket.read_some(buff_to_get_notification, 1);
	
	if (is_interrapted.is_lock_free()) {
		auto res = signal(SIGINT, signal_handler);
		if (res == SIG_ERR) {
			throw new socket_exception("signal failed\n");
		}
	} else {
		LOG("exception handling is not available\n");
	}
}

void IO_completion_port::close() {
	if (iocp_handle != INVALID_HANDLE_VALUE) {
		LOG("~~~~~~~~~~~~~\n");
		BOOL res = CloseHandle(iocp_handle);
		if (res == 0) {
			throw new socket_exception("Error in closing completion port : " + to_string(GetLastError()));
		}
	}
}

void IO_completion_port::registrate_socket(server_socket& sock) {
	completion_key *key = new completion_key(&sock);
	HANDLE res = CreateIoCompletionPort((HANDLE)sock.sd.get_sd(), iocp_handle, (DWORD)key, 0);
	if (res == NULL) {
		delete key;
		throw new socket_exception("CreateIoCompletionPort failed with error : " + to_string(GetLastError()) + "\n");
	}
	sock.key = key;
	key->num_referenses++;
}
void IO_completion_port::registrate_socket(client_socket& sock) {
	completion_key *key = new completion_key(&sock);
	HANDLE res = CreateIoCompletionPort((HANDLE)sock.sd.get_sd(), iocp_handle, (DWORD)key, 0);
	
	if (res == NULL) {
		delete key;
		throw new socket_exception("CreateIoCompletionPort failed with error : " + to_string(GetLastError()) + "\n");
	}
	sock.key = key;
	key->num_referenses++;
}
void IO_completion_port::registrate_timer(timer& t) {
	t.unregistrate();
	t.port = this;
	t.it = timers.emplace(&t);
	
	notify();
}
void IO_completion_port::registrate_on_interruption_event(func_t func) {
	on_interrupt_f.push_back(func);
}

void IO_completion_port::notify() {
	int res = send(to_notify.get_sd(), buff_to_notify, 1, 0);
	if (res == SOCKET_ERROR) {
		int error = WSAGetLastError();
		throw new socket_exception("send failed with error : " + to_string(error) + "\n");
	}
}

void IO_completion_port::add_task(func_t func) {
	lock_guard<mutex> lg(m);
	tasks.push_back(func);
	notify();
}

void IO_completion_port::start() {
	if (is_already_started.test_and_set()) {
		throw new socket_exception("IO_completion_port already started\n");
	}
	
	DWORD transmited_bytes;
	
	completion_key* received_key;
	OVERLAPPED* overlapped;
	
	socket_descriptor client;
	
	while (!is_interrapted.load()) {
		{
			lock_guard<mutex> lg(m);
			if (tasks.size() > 0) {
				func_t f = *tasks.begin();
				tasks.pop_front();
				try {
					f();
				} catch (...) {
					LOG("Exception in task to execute (IO_completion_port::start)\n");
				}
				continue;
			}
		}
		
		int time_to_wait = get_time_to_wait();
		while (time_to_wait == 0) {
			timer *t = (*timers.begin()).t;
			assert(timers.begin() == t->it);
			timers.erase(t->it);
			t->it = timers.end();
			
			try {
				t->on_time_expiration(t);
			} catch (...) {
				LOG("Exception in on_time_expiration\n");
				/// TODO
			}
			time_to_wait = get_time_to_wait();
		}
		
		int res = GetQueuedCompletionStatus(iocp_handle, &transmited_bytes, (LPDWORD)&received_key, 
										(LPOVERLAPPED*)&overlapped, time_to_wait);
		if (res == 0) {
			int error = GetLastError();
			if (error == 258) {
				LOG("-");
				continue;
			}
			throw new socket_exception("GetQueuedCompletionStatus failed with error "
										+ to_string(error) + "\n");
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
				
				socket_descriptor client = move(real_overlaped->sd);
				delete real_overlaped;
				
				((server_socket*)received_key->ptr)->on_accept(move(client));
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
												to_string(GetLastError()) + "\n");
				}
				
				if (type == client_socket::client_socket_overlapped::RECV_KEY) {
					real_ptr->on_read_completion(transmited_bytes);
					break;
				}
				if (type == client_socket::client_socket_overlapped::SEND_KEY) {
					real_ptr->on_write_completion(transmited_bytes);
					break;
				}
				throw new socket_exception("Uncnown operation code : " + to_string(type) + "\n");
			}
		}
	}
	termination();
}

IO_completion_port::~IO_completion_port() {
	try {
		close();
	} catch (const socket_exception *ex) {
		LOG("Exception in destructor of IO_completion_port");
	}
}
