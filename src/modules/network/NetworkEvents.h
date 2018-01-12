/**
 * @file
 */

#pragma once

#include "network/Network.h"
#include <enet/enet.h>
#include "core/Log.h"
#include "core/EventBus.h"

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

/**
 * @brief This event is thrown if a client drops the connection.
 *
 * @note Beware: This doesn't mean that the client is leaving the gameserver. It only means
 * that the disconnect phase has been initialized.
 */
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
