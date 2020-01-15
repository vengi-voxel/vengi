/**
 * @file
 */

#include "ServerMessages_generated.h"
#include "InitHandler.h"
#include "voxel/ClientPager.h"
#include "voxelworld/WorldEvents.h"

void InitHandler::execute(ENetPeer* peer, const void* raw) {
	const network::Init* message = getMsg<network::Init>(raw);
	const unsigned int seed = message->seed();
	Log::info("Initialize for seed %u", seed);
	_pager->setSeed(seed);
	_eventBus->publish(voxelworld::WorldCreatedEvent());
}
