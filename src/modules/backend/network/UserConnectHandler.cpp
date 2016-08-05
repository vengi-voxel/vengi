/**
 * @file
 */

#include "UserConnectHandler.h"
#include "messages/ClientMessages_generated.h"
#include "messages/ServerMessages_generated.h"
#include "backend/entity/User.h"
#include "core/Var.h"
#include "core/EMailValidator.h"

namespace backend {

UserConnectHandler::UserConnectHandler(network::NetworkPtr network, backend::EntityStoragePtr entityStorage, voxel::WorldPtr world) :
		_network(network), _entityStorage(entityStorage), _world(world) {
	auto data = CreateAuthFailed(_authFailed);
	auto msg = CreateServerMessage(_authFailed, network::messages::server::Type::AuthFailed, data.Union());
	FinishServerMessageBuffer(_authFailed, msg);
}

void UserConnectHandler::sendAuthFailed(ENetPeer* peer) {
	ENetPacket* packet = enet_packet_create(_authFailed.GetBufferPointer(), _authFailed.GetSize(), ENET_PACKET_FLAG_RELIABLE);
	_network->sendMessage(peer, packet);
}

void UserConnectHandler::execute(ENetPeer* peer, const void* raw) {
	const auto* message = getMsg<UserConnect>(raw);

	const std::string& email = message->email()->str();
	if (!core::isValidEmail(email)) {
		sendAuthFailed(peer);
		Log::warn("Invalid email given: '%s', %c", email.c_str(), email[0]);
		return;
	}
	const std::string& password = message->password()->str();
	if (password.empty()) {
		Log::warn("User tries to log into the gameserver without providing a password");
		sendAuthFailed(peer);
		return;
	}
	Log::info("User %s tries to log into the gameserver", email.c_str());

	UserPtr user = _entityStorage->login(peer, email, password);
	if (!user) {
		sendAuthFailed(peer);
		return;
	}

	Log::info("User '%s' logged into the gameserver", email.c_str());
	user->sendSeed(_world->seed());
	user->sendUserSpawn();
}

}
