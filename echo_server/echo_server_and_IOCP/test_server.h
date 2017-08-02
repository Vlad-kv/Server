#ifndef TEST_SERVER_H
#define TEST_SERVER_H

#include "abstract_server.h"
#include "getaddrinfo_executer.h"

class test_server : public abstract_server {
	vector<pair<long long, std::string>> requests = {
		{1, "ru.cppreference.com"},
		{2, "codeforces.com"},
		{3, "qwert"},
		{3, "qwer2.com"},
		{3, "ru"},
		{4, "vk.com"}
	};
public:
	getaddrinfo_executer executer;
	
	test_server(std::string addres_of_main_socket, int address_family, int type, int protocol, int port, IO_completion_port &comp_port) 
	: abstract_server(addres_of_main_socket, address_family, type, protocol, port, comp_port),
	  executer(comp_port, 1, 4, 2) {
		addrinfo hints;
		SecureZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		
		auto call_callback = [this](addrinfo *info) {
			callback(info);
		};
		for (auto &w : requests) {
			executer.execute(w.first, w.second, "", hints, call_callback);
		}
	}
	
	void on_accept(client_socket_2 client_s) override {
		LOG("in on_accept\n");
	}
	
	void on_interruption() override {
		LOG("test_server interrupted\n");
	}
	
	void callback(addrinfo *info) {
		if (info == nullptr) {
			LOG("not found\n");
		}
		INT iRetval;
		int i = 1;
		
		struct sockaddr_in  *sockaddr_ipv4;
		LPSOCKADDR sockaddr_ip;

		char ipstringbuffer[46];
		DWORD ipbufferlength = 46;
		
		for(addrinfo *ptr = info; ptr != nullptr; ptr = ptr->ai_next) {
			printf("getaddrinfo response %d\n", i++);
			printf("\tFlags: 0x%x\n", ptr->ai_flags);
			printf("\tFamily: ");
			switch (ptr->ai_family) {
				case AF_UNSPEC:
					printf("Unspecified\n");
					break;
				case AF_INET:
					printf("AF_INET (IPv4)\n");
					sockaddr_ipv4 = (struct sockaddr_in *) ptr->ai_addr;
					printf("\tIPv4 address %s\n",
						inet_ntoa(sockaddr_ipv4->sin_addr) );
					break;
				case AF_INET6:
					printf("AF_INET6 (IPv6)\n");
					sockaddr_ip = (LPSOCKADDR) ptr->ai_addr;
					ipbufferlength = 46;
					iRetval = WSAAddressToString(sockaddr_ip, (DWORD) ptr->ai_addrlen, NULL, 
						ipstringbuffer, &ipbufferlength );
					if (iRetval)
						printf("WSAAddressToString failed with %u\n", WSAGetLastError() );
					else    
						printf("\tIPv6 address %s\n", ipstringbuffer);
					break;
				case AF_NETBIOS:
					printf("AF_NETBIOS (NetBIOS)\n");
					break;
				default:
					printf("Other %ld\n", ptr->ai_family);
					break;
			}
			printf("\tProtocol: ");
			switch (ptr->ai_protocol) {
				case 0:
					printf("Unspecified\n");
					break;
				case IPPROTO_TCP:
					printf("IPPROTO_TCP (TCP)\n");
					break;
				case IPPROTO_UDP:
					printf("IPPROTO_UDP (UDP) \n");
					break;
				default:
					printf("Other %ld\n", ptr->ai_protocol);
					break;
			}
		}
		LOG("************************\n");
	}
};

#endif // TEST_SERVER_H
