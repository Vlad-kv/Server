#ifndef WSA_HOLDER_H
#define WSA_HOLDER_H

class WSA_holder {
public:
	WSAData wsaData;
	
	WSA_holder(WORD version) {
		int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    	if (res != 0) {
			throw new socket_exception("WSAStartup failed " + std::to_string(res) + "\n");
    	}
	}
	
	~WSA_holder() {
		WSACleanup();
	}
};

#endif // WSA_HOLDER_H
