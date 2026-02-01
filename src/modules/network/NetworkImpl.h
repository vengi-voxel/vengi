/**
 * @file
 * The pimpl header for platform-specific network stuff that is only included in the XXNetwork.cpp files
 */

#pragma once

#include "SocketId.h"

#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2spi.h>
#include <ws2tcpip.h>
#define network_cleanup() WSACleanup()
#define network_return int
#else
#define network_return ssize_t
#include <arpa/inet.h>
#include <fcntl.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#define closesocket close
#define network_cleanup()
#endif

namespace network {

static inline bool isValidSocketId(SocketId socket) {
#ifdef WIN32
	// on windows the SocketId can be any value except InvalidSocketId
	return socket != InvalidSocketId;
#else
	return socket != InvalidSocketId && socket < FD_SETSIZE;
#endif
}

// Platform-specific network implementation
struct NetworkImpl {
	SocketId socketFD = InvalidSocketId;
	fd_set readFDSet;
	fd_set writeFDSet;

	bool isValid() const {
		return isValidSocketId(socketFD);
	}

	NetworkImpl() {
		FD_ZERO(&readFDSet);
		FD_ZERO(&writeFDSet);
	}
};


} // namespace voxedit
