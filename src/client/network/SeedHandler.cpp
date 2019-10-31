/**
 * @file
 */

#include "ServerMessages_generated.h"
#include "SeedHandler.h"
#include "core/App.h"
#include "core/Var.h"
#include "voxelworld/WorldEvents.h"

void SeedHandler::execute(ENetPeer* peer, const void* raw) {
	const network::Seed* message = getMsg<network::Seed>(raw);
	const long seed = message->seed();
	_world->setSeed(seed);
	_eventBus->publish(voxelworld::WorldCreatedEvent(_world));
}
