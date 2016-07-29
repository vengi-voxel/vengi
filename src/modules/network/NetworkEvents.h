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
public:
	DisconnectEvent(ENetPeer* peer) :
			_peer(peer) {
		if (peer != nullptr) {
			Log::trace("Disconnect peer event %u", peer->connectID);
		} else {
			Log::trace("Could not connect");
		}
	}
	inline ENetPeer* peer() const {
		return _peer;
	}
};

}
