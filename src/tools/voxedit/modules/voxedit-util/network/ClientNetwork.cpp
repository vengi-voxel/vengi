/**
 * @file
 */

#include "ClientNetwork.h"
#include "NetworkError.h"
#include "NetworkImpl.h"
#include "ProtocolHandler.h"
#include "ProtocolHandlerRegistry.h"
#include "ProtocolMessage.h"
#include "ProtocolMessageFactory.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {
namespace network {

ClientNetwork::ClientNetwork(SceneManager *sceneMgr)
	: _impl(new NetworkImpl()), _voxelModificationHandler(sceneMgr), _nodeAddedHandler(sceneMgr),
	  _nodeKeyFramesHandle(sceneMgr), _nodeMovedHandler(sceneMgr), _nodePaletteChangedHandle(sceneMgr),
	  _nodePropertiesHandler(sceneMgr), _nodeRemovedHandler(sceneMgr), _nodeRenamedHandler(sceneMgr),
	  _sceneStateRequestHandler(sceneMgr), _sceneStateHandler(sceneMgr) {
}

ClientNetwork::~ClientNetwork() {
	shutdown();
	delete _impl;
}

void ClientNetwork::construct() {
	core::Var::get(cfg::VoxEditNetHostname, "localhost", _("The voxedit server hostname to connect to"));
}

void ClientNetwork::shutdown() {
	disconnect();
	_protocolRegistry.shutdown();
	network_cleanup();
}

void ClientNetwork::disconnect() {
	if (_impl->socketFD != InvalidSocketId) {
		closesocket(_impl->socketFD);
		_impl->socketFD = InvalidSocketId;
	}
	FD_ZERO(&_impl->readFDSet);
	FD_ZERO(&_impl->writeFDSet);
}

bool ClientNetwork::isConnected() const {
	return _impl->socketFD != InvalidSocketId;
}

bool ClientNetwork::connect(const core::String &hostname, uint16_t port) {
	if (_impl->socketFD != InvalidSocketId) {
		Log::warn("Already connected, disconnecting first");
		disconnect();
	}

	FD_ZERO(&_impl->readFDSet);
	FD_ZERO(&_impl->writeFDSet);

	struct addrinfo hints, *res = nullptr;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	core::String service = core::string::toString(port);
	int err = getaddrinfo(hostname.c_str(), service.c_str(), &hints, &res);
	if (err != 0 || res == nullptr) {
		Log::error("Failed to resolve hostname %s: %s", hostname.c_str(), getNetworkErrorString());
		return false;
	}

	_impl->socketFD = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (_impl->socketFD == InvalidSocketId) {
		freeaddrinfo(res);
		Log::error("Failed to create socket: %s", getNetworkErrorString());
		return false;
	}

	int connectResult = ::connect(_impl->socketFD, res->ai_addr, res->ai_addrlen);
	if (connectResult < 0) {
		closesocket(_impl->socketFD);
		_impl->socketFD = InvalidSocketId;
		freeaddrinfo(res);
		Log::error("Failed to connect to %s:%i: %s", hostname.c_str(), port, getNetworkErrorString());
		return false;
	}

	freeaddrinfo(res);
	FD_SET(_impl->socketFD, &_impl->readFDSet);
	return true;
}

bool ClientNetwork::init() {
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

	network::ProtocolHandlerRegistry &r = _protocolRegistry;
	r.registerHandler(network::PROTO_PING, &_nopHandler); // ping is just a nop for the client
	r.registerHandler(network::PROTO_COMMAND, &_nopHandler); // never execute commands on the client side
	r.registerHandler(network::PROTO_SCENE_STATE_REQUEST, &_sceneStateRequestHandler);
	r.registerHandler(network::PROTO_SCENE_STATE, &_sceneStateHandler);
	r.registerHandler(network::PROTO_VOXEL_MODIFICATION, &_voxelModificationHandler);
	r.registerHandler(network::PROTO_NODE_ADDED, &_nodeAddedHandler);
	r.registerHandler(network::PROTO_NODE_REMOVED, &_nodeRemovedHandler);
	r.registerHandler(network::PROTO_NODE_MOVED, &_nodeMovedHandler);
	r.registerHandler(network::PROTO_NODE_RENAMED, &_nodeRenamedHandler);
	r.registerHandler(network::PROTO_NODE_PALETTE_CHANGED, &_nodePaletteChangedHandle);
	r.registerHandler(network::PROTO_NODE_PROPERTIES, &_nodePropertiesHandler);
	r.registerHandler(network::PROTO_NODE_KEYFRAMES, &_nodeKeyFramesHandle);

	return true;
}

bool ClientNetwork::sendMessage(const ProtocolMessage &msg) {
	if (_impl->socketFD == InvalidSocketId) {
		return false;
	}

	const size_t total = msg.size();
	Log::debug("Send message of type %d with size %u to server", msg.getId(), (uint32_t)total);
	size_t sentTotal = 0;
	while (sentTotal < total) {
		const size_t toSend = total - sentTotal;
		const network_return sent = send(_impl->socketFD, (const char *)msg.getBuffer() + sentTotal, toSend, 0);
		if (sent < 0) {
			Log::warn("Failed to send message: %s", getNetworkErrorString());
			return false;
		}
		sentTotal += sent;
	}
	return true;
}

void ClientNetwork::update(double nowSeconds) {
	updateDelta(nowSeconds);
	if (_impl->socketFD == InvalidSocketId) {
		return;
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
	const int ready = select(_impl->socketFD + 1, &readFDsOut, &writeFDsOut, nullptr, &tv);
#endif
	if (ready < 0) {
		Log::error("select() failed: %s", getNetworkErrorString());
		return;
	}

	if (ready == 0) {
		// No sockets ready, nothing to process
		return;
	}

	if (!FD_ISSET(_impl->socketFD, &readFDsOut)) {
		// Socket is not ready for reading
		return;
	}
	core::Array<uint8_t, 16384> buf;
	const network_return len = recv(_impl->socketFD, (char *)&buf[0], buf.size(), 0);
	if (len < 0) {
		Log::error("Error receiving data: %s", getNetworkErrorString());
		disconnect();
		return;
	}
	if (len == 0) {
		Log::info("Connection closed by peer");
		disconnect();
		return;
	}
	Log::debug("Received %d bytes from server", (int)len);
	in.write(buf.data(), len);

	// Process all available messages in the buffer
	while (ProtocolMessageFactory::isNewMessageAvailable(in)) {
		core::ScopedPtr<ProtocolMessage> msg(ProtocolMessageFactory::create(in));
		if (!msg) {
			Log::warn("Received invalid message");
			break;
		}
		if (ProtocolHandler *handler = _protocolRegistry.getHandler(*msg)) {
			// ClientId is 0 - because we are the client, not the server
			handler->execute(0, *msg);
		} else {
			Log::error("No client handler for message type %d", (int)msg->getId());
		}
	}
}

} // namespace network
} // namespace voxedit
