#include <thread>
#include <cassert>
#include <signal.h>

#include "IO_completion_port.h"
using namespace std;

bool operator<(const timer_holder& t_1, const timer_holder& t_2) {
	return (*t_1.t < *t_2.t);
}

timer_holder::timer_holder(timer *t)
: t(t) {
}

///---------------------------
registration& registration::operator=(registration &&reg) {
	comp_port_ptr = move(reg.comp_port_ptr);
	on_interrupt_f_it = move(reg.on_interrupt_f_it);
	return *this;
}
registration::registration()
: on_interrupt_f_it(nullptr) {
}
registration::registration(registration &&reg)
: comp_port_ptr(move(reg.comp_port_ptr)), on_interrupt_f_it(move(reg.on_interrupt_f_it)) {
}
registration::registration(std::shared_ptr<IO_completion_port*> comp_port_ptr, std::list<std::function<void ()>>::iterator on_interrupt_f_it)
: comp_port_ptr(comp_port_ptr), on_interrupt_f_it(on_interrupt_f_it) {
}
registration::~registration() {
	if (comp_port_ptr != nullptr) {
		if ((*comp_port_ptr) != nullptr) {
			(*comp_port_ptr)->on_interrupt_f.erase(on_interrupt_f_it);
		}
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

void IO_completion_port::wait_to_comp_and_exec(int time_to_wait) {
	DWORD transmited_bytes;
	void* received_key;
	abstract_overlapped* overlapped;
	
	int res = GetQueuedCompletionStatus(iocp_handle, &transmited_bytes, (LPDWORD)&received_key, 
										(LPOVERLAPPED*)&overlapped, time_to_wait);
	
	int error = 0;
	if (res == 0) {
		error = GetLastError();
		if (error == WAIT_TIMEOUT) {
			LOG("-");
			return;
		}
	}
	LOG("\n");
	
	try {
		completion_key::key_ptr key = overlapped->key;
		key->on_comp(transmited_bytes, key, overlapped, error);
	} catch (...) {
		LOG("Exception in on_comp (IO_completion_port)\n");
	}
}

void IO_completion_port::termination() {
	is_terminated = true;
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
	notification_socket->close();
	
	while (completion_key::counter_of_items > 0) {
		LOG("waiting for completion of all operations (" << completion_key::counter_of_items << ")\n");
		wait_to_comp_and_exec(INFINITE);
	}
}

IO_completion_port::IO_completion_port()
: iocp_handle(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0)) {
	if (iocp_handle == NULL) {
		throw socket_exception("CreateIoCompletionPort failed with error : " + to_string(GetLastError()) + "\n");
	}
	is_already_started.clear();
	to_notify = socket_descriptor(AF_INET, SOCK_STREAM, 0);
	
	server_socket temp_s(AF_INET, SOCK_STREAM, 0, nullptr);
	registrate_socket(temp_s);
	
	temp_s.bind_and_listen(AF_INET, "127.0.0.1", 0);
	temp_s.accept(AF_INET, SOCK_STREAM, 0);
	
	sockaddr_in new_address = temp_s.get_sock_address();
	to_notify.connect(AF_INET, new_address.sin_addr.s_addr, htons(new_address.sin_port));
	
	DWORD transmited_bytes;
	void* received_key;
	abstract_overlapped* overlapped;
	
	int res = GetQueuedCompletionStatus(iocp_handle, &transmited_bytes, (LPDWORD)&received_key,
										(LPOVERLAPPED*)&overlapped, INFINITE);
	if (res == FALSE) {
		throw socket_exception("GetQueuedCompletionStatus failed with error "
										+ to_string(GetLastError()) + "\n");
	}
	
	server_socket::server_socket_overlapped *real_overlaped = 
												(server_socket::server_socket_overlapped*)overlapped;
	notification_socket = make_unique<client_socket>(move(real_overlaped->sd), 1);
	delete real_overlaped;
	
	notification_socket->set_on_read_completion(
		[this](const char* buff, int size) {
			if (size == 0) {
				
				/// TODO
			} else {
				LOG("Notification completed.\n");
				if (!is_terminated) {
					notification_socket->read_some();
				}
			}
		}
	);
	registrate_socket(*notification_socket);
	notification_socket->read_some();
	
	if (is_interrapted.is_lock_free()) {
		auto res = signal(SIGINT, signal_handler);
		if (res == SIG_ERR) {
			throw socket_exception("signal failed\n");
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
			throw socket_exception("Error in closing completion port : " + to_string(GetLastError()));
		}
	}
}

void IO_completion_port::registrate_socket(abstract_socket& sock) {
	if (sock.is_registrated) {
		throw socket_exception("socket already registrated\n");
	}
	if (!sock.sd.is_valid()) {
		throw socket_exception("socket not valid\n\n");
	}
	if (is_terminated) {
		throw socket_exception("IO_completion_port already terminated\n");
	}
	HANDLE res = CreateIoCompletionPort((HANDLE)sock.sd.get_sd(), iocp_handle, (ULONG_PTR)&(*sock.key), 0);
	if (res == NULL) {
		throw socket_exception("CreateIoCompletionPort failed with error : " + to_string(GetLastError()) + "\n");
	}
	sock.port_ptr = port_ptr;
	sock.is_registrated = true;
}
void IO_completion_port::registrate_timer(timer& t) {
	t.unregistrate();
	t.port = this;
	t.it = timers.emplace(&t);
	
	notify();
}
registration IO_completion_port::registrate_on_interruption_event(func_t func) {
	if (is_interrapted.load() == true) {
		throw socket_exception("IO_completion_port already interrupted\n");
	}
	on_interrupt_f.push_back(
		[func]() {
			try {
				func();
			} catch (...) {
				LOG("Exception in interruption_event\n");
			}
		}
	);
	return registration(port_ptr, (--on_interrupt_f.end()));
}

void IO_completion_port::interrupt() {
	is_interrapted.store(true);
}
void IO_completion_port::notify() {
	if (is_terminated) {
		return;
	}
	int res = send(to_notify.get_sd(), buff_to_notify, 1, 0);
	if (res == SOCKET_ERROR) {
		int error = WSAGetLastError();
		throw socket_exception("send failed with error : " + to_string(error) + "\n");
	}
}

void IO_completion_port::add_task(func_t func) {
	lock_guard<mutex> lg(m);
	tasks.push(func);
	notify();
}

void IO_completion_port::start() {
	if (is_already_started.test_and_set()) {
		throw socket_exception("IO_completion_port already started\n");
	}
	while (!is_interrapted.load()) {
		{
			func_t task_to_execute = nullptr;
			{
				lock_guard<mutex> lg(m);
				if (tasks.size() > 0) {
					task_to_execute = tasks.front();
					tasks.pop();
				}
			}
			if (task_to_execute != nullptr) {
				try {
					task_to_execute();
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
			}
			time_to_wait = get_time_to_wait();
		}
		wait_to_comp_and_exec(time_to_wait);
	}
	termination();
}

IO_completion_port::~IO_completion_port() {
	try {
		close();
	} catch (const socket_exception *ex) {
		LOG("Exception in destructor of IO_completion_port");
	}
	*port_ptr = nullptr;
}
