/**
 * @file
 */

#pragma once

#include "network/Network.h"
#include <enet/enet.h>
#include "core/Log.h"
#include "core/EventBus.h"

namespace network {

EVENTBUSPAYLOADEVENT(NewConnectionEvent, ENetPeer*);

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

	DisconnectEvent(): _peer(nullptr), _reason(DisconnectReason::Unknown) {}

public:
	EVENTBUSTYPEID(DisconnectEvent)

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
