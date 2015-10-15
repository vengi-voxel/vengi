#pragma once

#include "network/Network.h"
#include "backend/entity/EntityStorage.h"

#include <flatbuffers/flatbuffers.h>

namespace backend {

class UserConnectHandler: public network::IProtocolHandler {
private:
	network::NetworkPtr _network;
	backend::EntityStoragePtr _entityStorage;
	flatbuffers::FlatBufferBuilder _authFailed;

	void sendAuthFailed(ENetPeer* peer);

public:
	UserConnectHandler(network::NetworkPtr network, backend::EntityStoragePtr entityStorage);

	void execute(ENetPeer* peer, const void* message) override;
};

}
