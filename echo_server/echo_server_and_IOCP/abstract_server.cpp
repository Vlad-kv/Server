#include "abstract_server.h"

void free_data(my_completion_key *received_key, additional_info *data) {
	delete received_key;
	delete data;
}

void abstract_server::start_working(const IO_completion_port& iocp) {
	DWORD transmited_bytes;
	
	my_completion_key* received_key;
	additional_info* data;
	my_OVERLAPED* overlaped;
	
	socket_descriptor client;
	
	while (1) {
		int res = GetQueuedCompletionStatus(iocp.get_handle(), &transmited_bytes, (LPDWORD)&received_key, 
										(LPOVERLAPPED*)&overlaped, 500);
		if ((res == 0) && (GetLastError() == 258)) {
			LOG("-");
			continue;
		}
		LOG("\n");
		
		data = overlaped->info;
		int type_of_operation = overlaped->get_operation_type();
		client = std::move(overlaped->client);
		delete overlaped;
		
		if (res == 0) {
			if (GetLastError() == 64) {
				LOG("connection interrupted\n");
				LOG(received_key->get_sd() << "\n");
				
				free_data(received_key, data);
				continue;
			}
			
			LOG("Error in GetQueuedCompletionStatus : " << GetLastError() << "\n");
			return;
		}
		
		switch (type_of_operation) {
		case my_OVERLAPED::RECV_KEY :
			on_recv_completion(received_key, data, transmited_bytes);
			break;
		case my_OVERLAPED::SEND_KEY :
			on_send_completion(received_key, data, transmited_bytes);
			break;
		case my_OVERLAPED::ACCEPT_KEY : {
			my_completion_key* key = create_completion_key(std::move(client));
			CreateIoCompletionPort((HANDLE)key->get_sd(), iocp.get_handle(), (DWORD)key, 0);
			
			on_accept(key);
			accept();
			break;
		}
		default :
			throw new socket_exception("Unknown operation.");
		}
	}
}
