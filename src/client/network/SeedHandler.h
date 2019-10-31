/**
 * @file
 */

#pragma once

#include "network/Network.h"
#include "core/EventBus.h"

namespace voxelworld {
class WorldMgr;
typedef std::shared_ptr<WorldMgr> WorldMgrPtr;
}

/**
 * Handler that forwards the server seed to your world in order to recreate the same world
 */
class SeedHandler: public network::IProtocolHandler {
private:
	voxelworld::WorldMgrPtr _world;
	core::EventBusPtr _eventBus;
public:
	SeedHandler(const voxelworld::WorldMgrPtr& world, const core::EventBusPtr& eventBus) :
			_world(world), _eventBus(eventBus) {
	}

	void execute(ENetPeer* peer, const void* message) override;
};
