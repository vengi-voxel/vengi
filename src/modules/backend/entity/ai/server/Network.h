/**
 * @file
 */
#pragma once

#include "ai-shared/protocol/IProtocolHandler.h"
#include "ai-shared/protocol/IProtocolMessage.h"
#include "../common/Thread.h"
#include "core/String.h"
#include <stdint.h>
#include <list>
#ifdef WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2spi.h>
#else
#define SOCKET  int
#include <sys/select.h>
#endif

namespace backend {

class IProtocolMessage;

struct Client {
	explicit Client(SOCKET _socket) :
			socket(_socket), finished(false), in(), out() {
	}
	SOCKET socket;
	bool finished;
	ai::streamContainer in;
	ai::streamContainer out;
};

class INetworkListener {
public:
	virtual ~INetworkListener() {}

	virtual void onConnect(Client*) {}
	virtual void onDisconnect(Client*) {}
};

class Network {
protected:
	uint16_t _port;
	core::String _hostname;
	// the socket file descriptor
	SOCKET _socketFD;
	fd_set _readFDSet;
	fd_set _writeFDSet;
	int64_t _time;

	typedef std::list<Client> ClientSockets;
	typedef ClientSockets::iterator ClientSocketsIter;
	ClientSockets _clientSockets;
	ClientSocketsIter closeClient (ClientSocketsIter& i);

	typedef std::list<INetworkListener*> Listeners;
	Listeners _listeners;

	bool sendMessage(Client& client);
public:
	Network(uint16_t port = 10001, const core::String& hostname = "0.0.0.0");
	virtual ~Network();

	bool start();
	void update(int64_t deltaTime);

	void addListener(INetworkListener* listener);
	void removeListener(INetworkListener* listener);

	int getConnectedClients() const;

	/**
	 * @return @c false if there are no clients
	 */
	bool broadcast(const ai::IProtocolMessage& msg);
	bool sendToClient(Client* client, const ai::IProtocolMessage& msg);
};

inline int Network::getConnectedClients() const {
	return static_cast<int>(_clientSockets.size());
}

inline void Network::addListener(INetworkListener* listener) {
	_listeners.push_back(listener);
}

inline void Network::removeListener(INetworkListener* listener) {
	_listeners.remove(listener);
}

}
