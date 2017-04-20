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
	
	DWORD received_bytes;
	
	my_completion_key *received_key;
	additional_info *data;
	
	while (1) {
		if (GetQueuedCompletionStatus(iocp, &received_bytes, (LPDWORD)&received_key, 
										(LPOVERLAPPED *)&data, 500) == 0) {
			if (GetLastError() == 258) {
				LOG("-");
				continue;
			}
			LOG("\n");
			if (GetLastError() == 64) {
				LOG("connection interrupted\n");
				LOG(received_key->sd.get_sd() << "\n");
				
				free_data(received_key, data);
				continue;
			}
			
			LOG("Error in GetQueuedCompletionStatus : " << GetLastError() << "\n");
			return 0;
		}
		LOG("\n");
		
//		cout << received_key->sd.get_sd() << "\n";
		
		if (received_bytes == 0) {
			free_data(received_key, data);
			continue;
		}
		
		if (data->received_bytes == 0) {
			data->received_bytes = received_bytes;
		} else {
			data->sended_bytes += received_bytes;
		}
		
		if (data->sended_bytes < data->received_bytes) {
			data->data_buff.buf = data->buffer + data->sended_bytes;
			data->data_buff.len = data->received_bytes - data->sended_bytes;
			
			server->send(received_key, data);
		} else {
			server->recv(received_key, data);
		}
	}
}
