#include "abstract_server.h"

void free_data(my_completion_key *received_key, additional_info *data) {
	delete received_key;
	delete data;
}

DWORD WINAPI WorkingThread(LPVOID start_info) {
	abstract_server::mess_to_working_thread *mess = (abstract_server::mess_to_working_thread*)start_info;
	
	HANDLE iocp = mess->iocp;
	abstract_server *server = mess->server;
	
	delete mess;
	
	DWORD transmited_bytes;
	
	my_completion_key *received_key;
	additional_info *data;
	my_OVERLAPED *overlaped;
	
	while (1) {
		int res = GetQueuedCompletionStatus(iocp, &transmited_bytes, (LPDWORD)&received_key, 
										(LPOVERLAPPED *)&overlaped, 500);
		if ((res == 0) && (GetLastError() == 258)) {
			LOG("-");
			continue;
		}
		LOG("\n");
		
		data = overlaped->info;
		delete overlaped;
		
		if (res == 0) {
			if (GetLastError() == 64) {
				LOG("connection interrupted\n");
				LOG(received_key->sd.get_sd() << "\n");
				
				free_data(received_key, data);
				continue;
			}
			
			LOG("Error in GetQueuedCompletionStatus : " << GetLastError() << "\n");
			return 0;
		}
		if (data->get_operation_type() == additional_info::RECV_KEY) {
			server->on_recv_completion(received_key, data, transmited_bytes);
			continue;
		}
		if (data->get_operation_type() == additional_info::SEND_KEY) {
			server->on_send_completion(received_key, data, transmited_bytes);
			continue;
		}
		throw new socket_exception("Unknown operation.");
	}
}
