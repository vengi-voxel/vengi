/**
 * @file
 */

#pragma once

#include "network/Network.h"
#include "core/EventBus.h"

namespace client {
class ClientPager;
typedef std::shared_ptr<ClientPager> ClientPagerPtr;
}

/**
 * Handler that forwards the server seed to your world in order to recreate the same world
 */
class InitHandler: public network::IProtocolHandler {
private:
	client::ClientPagerPtr _pager;
	core::EventBusPtr _eventBus;
public:
	InitHandler(const client::ClientPagerPtr& pager, const core::EventBusPtr& eventBus) :
			_pager(pager), _eventBus(eventBus) {
	}

	void execute(ENetPeer* peer, const void* message) override;
};
