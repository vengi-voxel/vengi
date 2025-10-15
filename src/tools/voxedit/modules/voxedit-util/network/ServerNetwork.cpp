/**
 * @file
 */

#include "ServerNetwork.h"
#include "NetworkError.h"
#include "NetworkImpl.h"
#include "ProtocolHandler.h"
#include "ProtocolHandlerRegistry.h"
#include "ProtocolMessage.h"
#include "ProtocolMessageFactory.h"
#include "SocketId.h"
#include "app/I18N.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/Var.h"
#include "protocol/PingMessage.h"
#include "voxedit-util/Config.h"

namespace voxedit {
namespace network {

RemoteClient::RemoteClient(RemoteClient &&other) noexcept
	: socket(other.socket), bytesIn(other.bytesIn), bytesOut(other.bytesOut), lastPingTime(other.lastPingTime),
	  lastActivity(other.lastActivity), in(core::move(other.in)), out(core::move(other.out)),
	  name(core::move(other.name)) {
	other.socket = InvalidSocketId;
	other.bytesIn = 0u;
	other.bytesOut = 0u;
	other.lastPingTime = 0.0;
	other.lastActivity = 0.0;
}

RemoteClient &RemoteClient::operator=(RemoteClient &&other) noexcept {
	if (this != &other) {
		socket = other.socket;
		bytesIn = other.bytesIn;
		bytesOut = other.bytesOut;
		lastPingTime = other.lastPingTime;
		lastActivity = other.lastActivity;
		in = core::move(other.in);
		out = core::move(other.out);
		name = core::move(other.name);
		other.socket = InvalidSocketId;
		other.bytesIn = 0u;
		other.bytesOut = 0u;
		other.lastPingTime = 0.0;
		other.lastActivity = 0.0;
	}
	return *this;
}

ServerNetwork::ServerNetwork(Server *server)
	: _impl(new NetworkImpl()), _initSessionHandler(server), _sceneStateHandler(server), _broadcastHandler(server) {
}

ServerNetwork::~ServerNetwork() {
	shutdown();
	delete _impl;
}

void ServerNetwork::shutdown() {
	stop();
	_protocolRegistry.shutdown();
	network_cleanup();
}

bool ServerNetwork::start(uint16_t port, const core::String &iface) {
	FD_ZERO(&_impl->readFDSet);
	FD_ZERO(&_impl->writeFDSet);

	_impl->socketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_impl->socketFD == InvalidSocketId) {
		network_cleanup();
		Log::error("Failed to create socket: %s", getNetworkErrorString());
		return false;
	}
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;

	if (iface.empty() || iface == "0.0.0.0") {
		sin.sin_addr.s_addr = INADDR_ANY;
	} else {
		if (inet_pton(AF_INET, iface.c_str(), &sin.sin_addr) <= 0) {
			closesocket(_impl->socketFD);
			_impl->socketFD = InvalidSocketId;
			network_cleanup();
			Log::error("Invalid interface address '%s': %s", iface.c_str(), getNetworkErrorString());
			return false;
		}
	}
	sin.sin_port = htons(port);

	int t = 1;
#ifdef _WIN32
	if (setsockopt(_impl->socketFD, SOL_SOCKET, SO_REUSEADDR, (char *)&t, sizeof(t)) != 0) {
#else
	if (setsockopt(_impl->socketFD, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)) != 0) {
#endif
		closesocket(_impl->socketFD);
		_impl->socketFD = InvalidSocketId;
		network_cleanup();
		Log::error("Failed to set socket options: %s", getNetworkErrorString());
		return false;
	}

	if (bind(_impl->socketFD, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		FD_CLR(_impl->socketFD, &_impl->readFDSet);
		FD_CLR(_impl->socketFD, &_impl->writeFDSet);
		closesocket(_impl->socketFD);
		_impl->socketFD = InvalidSocketId;
		network_cleanup();
		const char *ifaceStr = sin.sin_addr.s_addr == INADDR_ANY ? "any interface" : iface.c_str();
		Log::error("Failed to bind to %s:%i: %s", ifaceStr, port, getNetworkErrorString());
		return false;
	}

	if (listen(_impl->socketFD, 5) < 0) {
		closesocket(_impl->socketFD);
		_impl->socketFD = InvalidSocketId;
		network_cleanup();
		const char *ifaceStr = sin.sin_addr.s_addr == INADDR_ANY ? "any interface" : iface.c_str();
		Log::error("Failed to listen on %s:%i: %s", ifaceStr, port, getNetworkErrorString());
		return false;
	}

#ifdef O_NONBLOCK
	fcntl(_impl->socketFD, F_SETFL, O_NONBLOCK);
#endif
#ifdef WIN32
	unsigned long mode = 1;
	ioctlsocket(_impl->socketFD, FIONBIO, &mode);
#endif

	FD_SET(_impl->socketFD, &_impl->readFDSet);
	return true;
}

void ServerNetwork::stop() {
	if (_impl->socketFD == InvalidSocketId) {
		return;
	}
	for (int j = _clients.size() - 1; j >= 0; --j) {
		disconnect((ClientId)j);
	}
	closesocket(_impl->socketFD);
	_impl->socketFD = InvalidSocketId;
}

bool ServerNetwork::isRunning() const {
	return _impl->socketFD != InvalidSocketId;
}

void ServerNetwork::construct() {
	core::Var::get(cfg::VoxEditNetPort, "10001", _("The port to run the voxedit server on"));
	core::Var::get(cfg::VoxEditNetPassword, "", core::CV_SECRET, _("The password required to connect to the voxedit server"));
	core::Var::get(cfg::VoxEditNetRconPassword, "changeme", core::CV_SECRET, _("The rcon password required to send commands to the voxedit server"));
	core::Var::get(cfg::VoxEditNetServerInterface, "0.0.0.0", _("The interface to run the voxedit server on"));
	_maxClients = core::Var::get(cfg::VoxEditNetServerMaxConnections, "10",
								 _("The maximum number of clients that can connect to the server"));
}

bool ServerNetwork::init() {
#ifdef WIN32
	WSADATA wsaData;
	const int wsaResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsaResult != NO_ERROR) {
		Log::error("WSAStartup failed with error: %d", wsaResult);
		return false;
	}
#else
	signal(SIGPIPE, SIG_IGN);
#endif

	ProtocolHandlerRegistry &r = _protocolRegistry;
	r.registerHandler(PROTO_INIT_SESSION, &_initSessionHandler);
	r.registerHandler(network::PROTO_PING, &_nopHandler);
	r.registerHandler(network::PROTO_COMMAND, &_commandHandler);
	r.registerHandler(network::PROTO_SCENE_STATE_REQUEST, &_broadcastHandler);
	r.registerHandler(network::PROTO_SCENE_STATE, &_sceneStateHandler);
	r.registerHandler(network::PROTO_VOXEL_MODIFICATION, &_broadcastHandler);
	r.registerHandler(network::PROTO_NODE_ADDED, &_broadcastHandler);
	r.registerHandler(network::PROTO_NODE_REMOVED, &_broadcastHandler);
	r.registerHandler(network::PROTO_NODE_MOVED, &_broadcastHandler);
	r.registerHandler(network::PROTO_NODE_RENAMED, &_broadcastHandler);
	r.registerHandler(network::PROTO_NODE_PALETTE_CHANGED, &_broadcastHandler);
	r.registerHandler(network::PROTO_NODE_PROPERTIES, &_broadcastHandler);
	r.registerHandler(network::PROTO_NODE_KEYFRAMES, &_broadcastHandler);
	return true;
}

void ServerNetwork::disconnect(ClientId clientId) {
	RemoteClient &client = _clients[clientId];
	const SocketId clientSocket = client.socket;
	FD_CLR(clientSocket, &_impl->readFDSet);
	FD_CLR(clientSocket, &_impl->writeFDSet);
	closesocket(clientSocket);
	client.socket = InvalidSocketId;
	Log::debug("RemoteClient %d disconnected", clientId);
	for (NetworkListener *listener : _listeners) {
		listener->onDisconnect(&client);
	}
	_clients.erase(clientId);
}

bool ServerNetwork::updateClient(RemoteClient &client) {
	const size_t total = client.out.size();
	size_t sentTotal = 0;
	while (sentTotal < total) {
		const size_t toSend = total - sentTotal;
		const network_return sent = send(client.socket, (const char *)client.out.getBuffer() + sentTotal, toSend, 0);
		if (sent < 0) {
			client.out.skip(sentTotal);
			client.out.trim();
			client.bytesOut += sentTotal;

#ifdef WIN32
			int error = WSAGetLastError();
			if (error == WSAEWOULDBLOCK || error == WSAEINPROGRESS) {
				return true;
			}
#else
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				return true;
			}
#endif
			// Real error occurred (connection reset, etc.)
			Log::error("Server send error: %s", getNetworkErrorString());
			return false;
		}
		if (sent == 0) {
			// Socket closed by peer
			Log::error("RemoteClient socket closed during send");
			return false;
		}
		sentTotal += sent;
	}
	client.out.skip(sentTotal);
	client.out.trim();
	client.bytesOut += sentTotal;

	return true;
}

void ServerNetwork::update(double nowSeconds) {
	updateDelta(nowSeconds);
	if (_impl->socketFD == InvalidSocketId) {
		return;
	}
	_pingSeconds += _deltaSeconds;
	if (_pingSeconds > 5.0) {
		PingMessage msg;
		core_assert(msg.size() == 5);
		// broadcast(msg);
		_pingSeconds = 0.0;
	}
	fd_set readFDsOut;
	fd_set writeFDsOut;

	readFDsOut = _impl->readFDSet;
	writeFDsOut = _impl->writeFDSet;

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
#ifdef WIN32
	const int ready = select(0, &readFDsOut, &writeFDsOut, nullptr, &tv);
#else
	int maxFd = _impl->socketFD;
	for (const auto &client : _clients) {
		const SocketId clientSocket = client.socket;
		if (client.socket != InvalidSocketId && clientSocket > maxFd) {
			maxFd = clientSocket;
		}
	}
	const int ready = select(maxFd + 1, &readFDsOut, &writeFDsOut, nullptr, &tv);
#endif
	if (ready < 0) {
		Log::warn("select() failed: %s", getNetworkErrorString());
		return;
	}
	if (_impl->socketFD != InvalidSocketId && FD_ISSET(_impl->socketFD, &readFDsOut)) {
		const SocketId clientSocket = accept(_impl->socketFD, nullptr, nullptr);
		if (clientSocket != InvalidSocketId) {
			if (_clients.size() >= (size_t)_maxClients->intVal()) {
				Log::info("Maximum number of clients reached - rejecting connection");
				closesocket(clientSocket);
				return;
			}
			FD_SET(clientSocket, &_impl->readFDSet);
			_clients.emplace_back(static_cast<uintptr_t>(clientSocket));
			RemoteClient *client = &_clients.back();
			client->lastActivity = nowSeconds;
			client->lastPingTime = nowSeconds;
			for (NetworkListener *listener : _listeners) {
				listener->onConnect(client);
			}
		}
	}

	ClientId clientId = 0;
	core::Array<bool, 256> remove;
	remove.fill(false);
	for (auto i = _clients.begin(); i != _clients.end(); ++i, ++clientId) {
		RemoteClient &client = *i;
		const SocketId clientSocket = client.socket;
		if (client.socket == InvalidSocketId) {
			remove[clientId] = true;
			continue;
		}

		if (FD_ISSET(clientSocket, &writeFDsOut)) {
			if (!updateClient(client)) {
				remove[clientId] = true;
				continue;
			}
		}

		if (FD_ISSET(clientSocket, &readFDsOut)) {
			core::Array<uint8_t, 16384> buf;
			const network_return len = recv(clientSocket, (char *)&buf[0], buf.size(), 0);
			if (len < 0) {
				Log::debug("RemoteClient %d recv error: %s", clientId, getNetworkErrorString());
				remove[clientId] = true;
				continue;
			} else if (len == 0) {
				Log::debug("RemoteClient %d disconnected gracefully", clientId);
				remove[clientId] = true;
				continue;
			}
			// Update activity timestamp when we receive data
			client.lastActivity = nowSeconds;
			client.bytesIn += len;
			Log::debug("Received %d bytes from client %d", (int)len, clientId);
			Log::trace("first bytes: %02x %02x %02x %02x %02x", buf[0], buf[1], buf[2], buf[3], buf[4]);
			client.in.write(buf.data(), len);
		}

		// Process all available messages in the buffer
		while (ProtocolMessageFactory::isNewMessageAvailable(client.in)) {
			core::ScopedPtr<ProtocolMessage> msg(ProtocolMessageFactory::create(client.in));
			if (!msg) {
				Log::debug("RemoteClient %d sent invalid message", clientId);
				remove[clientId] = true;
				break;
			}
			// Update activity timestamp when we receive a valid message
			client.lastActivity = nowSeconds;
			if (ProtocolHandler *handler = _protocolRegistry.getHandler(*msg)) {
				handler->execute(clientId, *msg);
			} else {
				Log::warn("No server handler for message type %d", msg->getId());
			}
		}
	}
	if (clientId > 0) {
		for (size_t j = clientId; j > 0u; --j) {
			if (!remove[j - 1]) {
				continue;
			}
			disconnect((ClientId)j - 1);
		}
	}
}

bool ServerNetwork::broadcast(ProtocolMessage &msg, ClientId except) {
	if (_clients.empty()) {
		return false;
	}
	_pingSeconds = 0.0;
	core::Array<bool, 256> remove;
	remove.fill(false);
	ClientId clientId = 0;
	for (auto i = _clients.begin(); i != _clients.end(); ++i, ++clientId) {
		if (clientId == except) {
			continue;
		}
		RemoteClient &client = *i;
		if (client.socket == InvalidSocketId) {
			remove[clientId] = true;
			continue;
		}

		Log::debug("Broadcasting message to client %i", (int)clientId);

		msg.seek(0);
		client.out.seek(0, SEEK_END);
		client.out.writeStream(msg);
		FD_SET(client.socket, &_impl->writeFDSet);
	}
	if (clientId > 0) {
		for (size_t j = clientId; j > 0u; --j) {
			if (!remove[j - 1]) {
				continue;
			}
			disconnect((ClientId)j - 1);
		}
	}

	return true;
}

bool ServerNetwork::sendToClient(RemoteClient &client, ProtocolMessage &msg) {
	if (client.socket == InvalidSocketId) {
		return false;
	}

	msg.seek(0);
	client.out.seek(0, SEEK_END);
	client.out.writeStream(msg);
	FD_SET(client.socket, &_impl->writeFDSet);
	return true;
}

bool ServerNetwork::sendToClient(ClientId clientId, ProtocolMessage &msg) {
	if (clientId >= (ClientId)_clients.size()) {
		Log::error("Invalid client ID %d - failed to send message: %s", clientId, getNetworkErrorString());
		return false;
	}
	return sendToClient(_clients[clientId], msg);
}

void ServerNetwork::addListener(NetworkListener *listener) {
	core_assert(listener != nullptr);
	_listeners.push_back(listener);
}

void ServerNetwork::removeListener(NetworkListener *listener) {
	auto it = core::find(_listeners.begin(), _listeners.end(), listener);
	if (it != _listeners.end()) {
		_listeners.erase(it);
	}
}

} // namespace network
} // namespace voxedit
