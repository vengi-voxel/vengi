#include "UserConnectHandler.h"
#include "network/messages/ClientMessages.h"
#include "network/messages/ServerMessages.h"
#include "backend/entity/User.h"
#include "core/Var.h"

namespace backend {

UserConnectHandler::UserConnectHandler(network::NetworkPtr network, backend::EntityStoragePtr entityStorage) :
		_network(network), _entityStorage(entityStorage) {
	auto data = CreateAuthFailed(_authFailed);
	auto msg = CreateServerMessage(_authFailed, Type_AuthFailed, data.Union());
	FinishServerMessageBuffer(_authFailed, msg);
}

void UserConnectHandler::sendAuthFailed(ENetPeer* peer) {
	ENetPacket* packet = enet_packet_create(_authFailed.GetBufferPointer(), _authFailed.GetSize(), ENET_PACKET_FLAG_RELIABLE);
	_network->sendMessage(peer, packet);
}

void UserConnectHandler::execute(ENetPeer* peer, const void* raw) {
	const auto* message = getMsg<UserConnect>(raw);

	const char *email = message->email()->c_str();
	const char *password = message->password()->c_str();
	Log::info("User %s tries to log into the gameserver", email);

	UserPtr user = _entityStorage->login(peer, email, password);
	if (!user) {
		sendAuthFailed(peer);
		return;
	}

	// TODO: use the current seed from the world, not from a config var that can be changed during runtime
	const long seed = core::Var::get("sv_seed")->longVal();
	user->sendSeed(seed);
	user->sendUserSpawn();
}

}
