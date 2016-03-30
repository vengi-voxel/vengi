#include "SeedHandler.h"
#include "core/Var.h"
#include "network/messages/ServerMessages.h"
#include "voxel/World.h"

void SeedHandler::execute(ENetPeer* peer, const void* raw) {
	const network::messages::server::Seed* message = static_cast<const network::messages::server::Seed*>(raw);
	const long seed = message->seed();
	_world->setSeed(seed);
}
