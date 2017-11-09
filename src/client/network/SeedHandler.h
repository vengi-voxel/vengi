/**
 * @file
 */

#pragma once

#include "network/Network.h"
#include "core/EventBus.h"

namespace voxel {
class World;
typedef std::shared_ptr<World> WorldPtr;
}

/**
 * Handler that forwards the server seed to your world in order to recreate the same world
 */
class SeedHandler: public network::IProtocolHandler {
private:
	voxel::WorldPtr _world;
	core::EventBusPtr _eventBus;
public:
	SeedHandler(const voxel::WorldPtr& world, const core::EventBusPtr& eventBus) :
			_world(world), _eventBus(eventBus) {
	}

	void execute(ENetPeer* peer, const void* message) override;
};
