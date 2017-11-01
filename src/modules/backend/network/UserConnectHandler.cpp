/**
 * @file
 */

#include "ClientMessages_generated.h"
#include "ServerMessages_generated.h"
#include "UserConnectHandler.h"
#include "backend/entity/User.h"
#include "core/Var.h"
#include "util/EMailValidator.h"

namespace backend {

UserConnectHandler::UserConnectHandler(const network::NetworkPtr& network,
		const backend::EntityStoragePtr& entityStorage) :
		_network(network), _entityStorage(entityStorage) {
	auto data = network::CreateAuthFailed(_authFailed);
	auto msg = network::CreateServerMessage(_authFailed, network::ServerMsgType::AuthFailed, data.Union());
	network::FinishServerMessageBuffer(_authFailed, msg);
}

void UserConnectHandler::sendAuthFailed(ENetPeer* peer) {
	ENetPacket* packet = enet_packet_create(_authFailed.GetBufferPointer(), _authFailed.GetSize(), ENET_PACKET_FLAG_RELIABLE);
	_network->sendMessage(peer, packet);
}

void UserConnectHandler::execute(ENetPeer* peer, const void* raw) {
	const auto* message = getMsg<network::UserConnect>(raw);

	const std::string& email = message->email()->str();
	if (!util::isValidEmail(email)) {
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

	const UserPtr& user = _entityStorage->login(peer, email, password);
	if (!user) {
		sendAuthFailed(peer);
		return;
	}

	Log::info("User '%s' logged into the gameserver", email.c_str());
	const long seed = core::Var::getSafe(cfg::ServerSeed)->longVal();
	user->sendSeed(seed);
	user->sendUserSpawn();
}

}
