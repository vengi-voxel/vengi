/**
 * @file
 */

#include "Network.h"
#include "Network.cpp.h"

bool networkInit() {
	#ifdef WIN32
	WSADATA wsaData;
	const int wsaResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (wsaResult != NO_ERROR) {
		return false;
	}
#else
	signal(SIGPIPE, SIG_IGN);
#endif
	return true;
}

void networkNonBlocking(SOCKET socket) {
#ifdef O_NONBLOCK
	fcntl(socket, F_SETFL, O_NONBLOCK);
#endif
#ifdef WIN32
	unsigned long mode = 1;
	ioctlsocket(socket, FIONBIO, &mode);
#endif
}
