#pragma once

#include <enet/enet.h>

namespace network {

class NewConnectionEvent: public core::IEventBusEvent {
private:
	ENetPeer* _peer;
public:
	NewConnectionEvent(ENetPeer* peer) :
			_peer(peer) {
	}
	inline ENetPeer* peer() const {
		return _peer;
	}
};

class DisconnectEvent: public core::IEventBusEvent {
private:
	ENetPeer* _peer;
public:
	DisconnectEvent(ENetPeer* peer) :
			_peer(peer) {
	}
	inline ENetPeer* peer() const {
		return _peer;
	}
};

}
