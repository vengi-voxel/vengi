#pragma once

#include "network/Network.h"
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
public:
	SeedHandler(voxel::WorldPtr world) : _world(world) {}

	void execute(ENetPeer* peer, const void* message) override;
};
