#include "sockets_wrapper.h"
#include "../echo_server_and_IOCP/logger.h"

const int BUFFER_SIZE = 1024;
char buffer[BUFFER_SIZE];

client_socket accept_socket(const client_socket& sock) {
	SOCKET res = accept(sock.get_sd(), 0, 0);
	if (res == INVALID_SOCKET) {
		throw new socket_exception("Accept failed with error " + to_str(WSAGetLastError()) + "\n");
	}
	return client_socket(res);
}

void bind_socket(const server_socket& sock, short family, u_long addr, u_short port) {
	sockaddr_in addres;
	
	addres.sin_family = family;
	addres.sin_addr.s_addr = addr;
	addres.sin_port = port;
	
	int res = bind(sock.get_sd(), (SOCKADDR *) &addres, sizeof(addres));
	
	if (res == SOCKET_ERROR) {
		throw new socket_exception("Bind failed with error " + to_str(WSAGetLastError()) + "\n");
	}
}

void connect_to_socket(const socket_descriptor& connectSocket, short family, u_long addr, u_short port) {
	sockaddr_in addres;
	
	addres.sin_family = family;
	addres.sin_addr.s_addr = addr;
	addres.sin_port = port;
	
	int res = connect(connectSocket.get_sd(), (SOCKADDR *) &addres, sizeof(addres));
	if (res == SOCKET_ERROR) {
		throw new socket_exception("Connect function failed with error " + to_str(WSAGetLastError()) + "\n");
	}
}

void send_to_socket(const client_socket& sock, string mess) {
	size_t sended_bytes = 0;
	while (sended_bytes < mess.length()) {
		int res = send(sock.get_sd(), (&mess[0]) + sended_bytes, mess.length() - sended_bytes, 0);
		if (res == SOCKET_ERROR) {
			throw new socket_exception("Send failed with error " + to_str(WSAGetLastError()) + "\n");
		}
		
		LOG("Bytes send : " << res << "\n");
		sended_bytes += res;
	}
}

void blocking_send(const socket_descriptor& sock, string mess) {
	size_t global_sended_bytes = 0;
	WSABUF data_buf;
	WSAOVERLAPPED send_overlapped;
	ZeroMemory((PVOID) & send_overlapped, sizeof (WSAOVERLAPPED));
	
	send_overlapped.hEvent = WSACreateEvent();
	if (send_overlapped.hEvent == NULL) {
		throw new socket_exception("WSACreateEvent failed with error : " +
									to_str(WSAGetLastError()) + "\n");
	}
	
	while (global_sended_bytes < mess.length()) {
		data_buf.len = mess.length() - global_sended_bytes;
		data_buf.buf = &mess[global_sended_bytes];
		int res;
		
		DWORD sended_bytes;
		
		res = WSASend(sock.get_sd(), &data_buf, 1, &sended_bytes,
						0, &send_overlapped, NULL);
		if ((res == SOCKET_ERROR) && ( (res = WSAGetLastError()) != WSA_IO_PENDING )) {
			throw new socket_exception("WSASend failed with error : " +
										to_str(res) + "\n");
		}
		res = WSAWaitForMultipleEvents(1, &send_overlapped.hEvent, TRUE, INFINITE, TRUE);
		if (res == WSA_WAIT_FAILED) {
			throw new socket_exception("WSAWaitForMultipleEvents failed with error : " +
										to_str(WSAGetLastError()) + "\n");
		}
		DWORD flags = 0;
		res = WSAGetOverlappedResult(sock.get_sd(), &send_overlapped, &sended_bytes,
										FALSE, &flags);
		if (res == FALSE) {
			throw new socket_exception("WSAGetOverlappedResult failed with error : " +
										to_str(WSAGetLastError()) + "\n");
		}
		WSAResetEvent(send_overlapped.hEvent);
		
		LOG("Sended " + to_str(sended_bytes) + " ");
		
		global_sended_bytes += sended_bytes;
		
		LOG(to_str(global_sended_bytes) + "/" + to_str(mess.length()) + "\n");
	}
	LOG("blocking_send finished\n");
}

string receive_from_socket(const socket_descriptor& sock) {
	string result;
	
	int received_bytes;
	
	do {
		received_bytes = recv(sock.get_sd(), buffer, BUFFER_SIZE, 0);
		
		LOG("Received bytes - " << received_bytes << " : ");
		
		for (int w = 0; w < received_bytes; w++) {
			result.push_back(buffer[w]);
			LOG(buffer[w]);
		}
		LOG("\n");
	} while (received_bytes > 0);
	
	return result;
}
