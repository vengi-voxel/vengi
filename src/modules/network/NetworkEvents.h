/**
 * @file
 */

#pragma once

#include <enet/enet.h>
#include "core/Log.h"

namespace network {

class NewConnectionEvent: public core::IEventBusEvent {
private:
	ENetPeer* _peer;
public:
	NewConnectionEvent(ENetPeer* peer) :
			_peer(peer) {
		Log::trace("Connect peer event %u", peer->connectID);
	}
	inline ENetPeer* peer() const {
		return _peer;
	}
};

class DisconnectEvent: public core::IEventBusEvent {
private:
	ENetPeer* _peer;
	DisconnectReason _reason;
public:
	DisconnectEvent(ENetPeer* peer, DisconnectReason reason) :
			_peer(peer), _reason(reason) {
	}

	inline DisconnectReason reason() const {
		return _reason;
	}

	inline ENetPeer* peer() const {
		return _peer;
	}
};

}
