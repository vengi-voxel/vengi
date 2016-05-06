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
		Log::info("Connect peer %i", peer->connectID);
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
		Log::info("Disconnect peer %i", peer->connectID);
	}
	inline ENetPeer* peer() const {
		return _peer;
	}
};

}
