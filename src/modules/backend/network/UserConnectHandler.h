/**
 * @file
 */

#pragma once

#include "network/Network.h"
#include "voxel/World.h"
#include "backend/entity/EntityStorage.h"

#include <flatbuffers/flatbuffers.h>

namespace backend {

class UserConnectHandler: public network::IProtocolHandler {
private:
	network::NetworkPtr _network;
	backend::EntityStoragePtr _entityStorage;
	voxel::WorldPtr _world;
	flatbuffers::FlatBufferBuilder _authFailed;

	void sendAuthFailed(ENetPeer* peer);

public:
	UserConnectHandler(const network::NetworkPtr& network,
			const backend::EntityStoragePtr& entityStorage, const voxel::WorldPtr& world);

	void execute(ENetPeer* peer, const void* message) override;
};

}
