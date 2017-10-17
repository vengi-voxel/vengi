/**
 * @file
 */

#include "ServerMessages_generated.h"
#include "SeedHandler.h"
#include "core/App.h"
#include "core/Var.h"
#include "voxel/WorldEvents.h"

void SeedHandler::execute(ENetPeer* peer, const void* raw) {
	const network::Seed* message = getMsg<network::Seed>(raw);
	const long seed = message->seed();
	_world->setSeed(seed);
	core::App::getInstance()->eventBus()->publish(voxel::WorldCreatedEvent(_world));
}
