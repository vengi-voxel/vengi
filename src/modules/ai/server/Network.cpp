/**
 * @file
 */

#include "Network.h"
#include "core/Common.h"
#include "core/StandardLib.h"
#include "IProtocolMessage.h"
#include "IProtocolHandler.h"
#include "ProtocolHandlerRegistry.h"
#include "ProtocolMessageFactory.h"
#ifdef WIN32
#define network_cleanup() WSACleanup()
#define network_return int
#else
#define network_return ssize_t
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netdb.h>
#include <signal.h>
#define closesocket close
#define INVALID_SOCKET  -1
#define network_cleanup()
#endif
#include <string.h>
#include <deque>
#include <algorithm>
#include <assert.h>
#include <stddef.h>
#include <memory>
#include <iterator>
#include "core/collection/Array.h"

namespace ai {

Network::Network(uint16_t port, const core::String& hostname) :
		_port(port), _hostname(hostname), _socketFD(INVALID_SOCKET), _time(0L) {
	FD_ZERO(&_readFDSet);
	FD_ZERO(&_writeFDSet);
}

Network::~Network() {
	closesocket(_socketFD);
	network_cleanup();
}

bool Network::start() {
#ifdef WIN32
	WSADATA wsaData;
	const int wsaResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (wsaResult != NO_ERROR) {
		return false;
	}
#else
	signal(SIGPIPE, SIG_IGN);
#endif

	FD_ZERO(&_readFDSet);
	FD_ZERO(&_writeFDSet);

	_socketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_socketFD == INVALID_SOCKET) {
		network_cleanup();
		return false;
	}
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(_port);

	int t = 1;
#ifdef _WIN32
	if (setsockopt(_socketFD, SOL_SOCKET, SO_REUSEADDR, (char*) &t, sizeof(t)) != 0) {
#else
	if (setsockopt(_socketFD, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)) != 0) {
#endif
		closesocket(_socketFD);
		return false;
	}

	if (bind(_socketFD, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
		// Handle the error.
		network_cleanup();
		FD_CLR(_socketFD, &_readFDSet);
		FD_CLR(_socketFD, &_writeFDSet);
		closesocket(_socketFD);
		_socketFD = INVALID_SOCKET;
		return false;
	}

	if (listen(_socketFD, 5) < 0) {
		// Handle the error.
		network_cleanup();
		closesocket(_socketFD);
		_socketFD = INVALID_SOCKET;
		return false;
	}

#ifdef O_NONBLOCK
	fcntl(_socketFD, F_SETFL, O_NONBLOCK);
#endif
#ifdef WIN32
	unsigned long mode = 1;
	ioctlsocket(_socketFD, FIONBIO, &mode);
#endif

	FD_SET(_socketFD, &_readFDSet);

	return true;
}

Network::ClientSocketsIter Network::closeClient(ClientSocketsIter& iter) {
	Client& client = *iter;
	const SOCKET clientSocket = client.socket;
	FD_CLR(clientSocket, &_readFDSet);
	FD_CLR(clientSocket, &_writeFDSet);
	closesocket(clientSocket);
	client.socket = INVALID_SOCKET;
	for (INetworkListener* listener : _listeners) {
		listener->onDisconnect(&client);
	}
	return _clientSockets.erase(iter);
}

bool Network::sendMessage(Client& client) {
	if (client.out.empty()) {
		return true;
	}

	core::Array<uint8_t, 16384> buf;
	while (!client.out.empty()) {
		const size_t len = core_min(buf.size(), client.out.size());
		std::copy_n(client.out.begin(), len, buf.begin());
		const SOCKET clientSocket = client.socket;
		const network_return sent = send(clientSocket, (const char*)&buf[0], len, 0);
		if (sent < 0) {
			return false;
		}
		if (sent == 0) {
			// better luck next time - but don't block others
			return true;
		}
		client.out.erase(client.out.begin(), std::next(client.out.begin(), sent));
	}
	return true;
}

void Network::update(int64_t deltaTime) {
	_time += deltaTime;
	if (_time > 5000L) {
		if (!broadcast(AIPingMessage())) {
			_time = 0L;
		}
	}
	fd_set readFDsOut;
	fd_set writeFDsOut;

	core_memcpy(&readFDsOut, &_readFDSet, sizeof(readFDsOut));
	core_memcpy(&writeFDsOut, &_writeFDSet, sizeof(writeFDsOut));

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	const int ready = select(FD_SETSIZE, &readFDsOut, &writeFDsOut, nullptr, &tv);
	if (ready < 0) {
		return;
	}
	if (_socketFD != INVALID_SOCKET && FD_ISSET(_socketFD, &readFDsOut)) {
		const SOCKET clientSocket = accept(_socketFD, nullptr, nullptr);
		if (clientSocket != INVALID_SOCKET) {
			FD_SET(clientSocket, &_readFDSet);
			const Client c(clientSocket);
			_clientSockets.push_back(c);
			for (INetworkListener* listener : _listeners) {
				listener->onConnect(&_clientSockets.back());
			}
		}
	}

	ClientId clientId = 0;
	for (ClientSocketsIter i = _clientSockets.begin(); i != _clientSockets.end(); ++clientId) {
		Client& client = *i;
		const SOCKET clientSocket = client.socket;
		if (clientSocket == INVALID_SOCKET) {
			i = closeClient(i);
			continue;
		}

		if (FD_ISSET(clientSocket, &writeFDsOut)) {
			if (!sendMessage(client) || client.finished) {
				i = closeClient(i);
				continue;
			}
		}

		if (FD_ISSET(clientSocket, &readFDsOut)) {
			core::Array<uint8_t, 16384> buf;
			const network_return len = recv(clientSocket, (char*)&buf[0], buf.size(), 0);
			if (len < 0) {
				i = closeClient(i);
				continue;
			}
			std::copy_n(buf.begin(), len, std::back_inserter(client.in));
		}

		ProtocolMessageFactory& factory = ProtocolMessageFactory::get();
		if (factory.isNewMessageAvailable(client.in)) {
			IProtocolMessage* msg = factory.create(client.in);
			if (!msg) {
				i = closeClient(i);
				continue;
			}
			IProtocolHandler* handler = ProtocolHandlerRegistry::get().getHandler(*msg);
			if (handler) {
				handler->execute(clientId, *msg);
			}
		}
		++i;
	}
}

bool Network::broadcast(const IProtocolMessage& msg) {
	if (_clientSockets.empty()) {
		return false;
	}
	_time = 0L;
	streamContainer out;
	msg.serialize(out);
	for (ClientSocketsIter i = _clientSockets.begin(); i != _clientSockets.end(); ++i) {
		Client& client = *i;
		if (client.socket == INVALID_SOCKET) {
			i = closeClient(i);
			continue;
		}

		IProtocolMessage::addInt(client.out, static_cast<int32_t>(out.size()));
		std::copy(out.begin(), out.end(), std::back_inserter(client.out));
		FD_SET(client.socket, &_writeFDSet);
	}

	return true;
}

bool Network::sendToClient(Client* client, const IProtocolMessage& msg) {
	assert(client != nullptr);
	if (client->socket == INVALID_SOCKET) {
		return false;
	}

	streamContainer out;
	msg.serialize(out);

	IProtocolMessage::addInt(client->out, static_cast<int32_t>(out.size()));
	std::copy(out.begin(), out.end(), std::back_inserter(client->out));
	FD_SET(client->socket, &_writeFDSet);
	return true;
}

#undef network_cleanup
#undef INVALID_SOCKET
#ifndef WIN32
#undef closesocket
#endif

}
