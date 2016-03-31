#include "SeedHandler.h"
#include "core/App.h"
#include "core/Var.h"
#include "network/messages/ServerMessages.h"
#include "voxel/WorldEvents.h"

void SeedHandler::execute(ENetPeer* peer, const void* raw) {
	const network::messages::server::Seed* message = getMsg<network::messages::server::Seed>(raw);
	const long seed = message->seed();
	_world->setSeed(seed);
	core::App::getInstance()->eventBus()->publish(voxel::WorldCreatedEvent(_world));
}
