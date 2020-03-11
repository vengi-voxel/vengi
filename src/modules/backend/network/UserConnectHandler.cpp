/**
 * @file
 */

#include "ClientMessages_generated.h"
#include "ServerMessages_generated.h"
#include "UserConnectHandler.h"
#include "backend/entity/User.h"
#include "core/Var.h"
#include "core/Log.h"
#include "util/EMailValidator.h"
#include "UserModel.h"
#include "backend/entity/EntityStorage.h"
#include "backend/world/MapProvider.h"
#include "backend/world/Map.h"
#include "attrib/ContainerProvider.h"

namespace backend {

UserConnectHandler::UserConnectHandler(
		const network::NetworkPtr& network,
		const MapProviderPtr& mapProvider,
		const persistence::DBHandlerPtr& dbHandler,
		const persistence::PersistenceMgrPtr& persistenceMgr,
		const backend::EntityStoragePtr& entityStorage,
		const network::ServerMessageSenderPtr& messageSender,
		const core::TimeProviderPtr& timeProvider,
		const attrib::ContainerProviderPtr& containerProvider,
		const cooldown::CooldownProviderPtr& cooldownProvider,
		const stock::StockDataProviderPtr& stockDataProvider) :
		_network(network), _mapProvider(mapProvider), _dbHandler(dbHandler), _persistenceMgr(persistenceMgr),
		_entityStorage(entityStorage), _messageSender(messageSender), _timeProvider(timeProvider),
		_containerProvider(containerProvider), _cooldownProvider(cooldownProvider),
		_stockDataProvider(stockDataProvider) {
	auto data = network::CreateAuthFailed(_authFailed);
	auto msg = network::CreateServerMessage(_authFailed, network::ServerMsgType::AuthFailed, data.Union());
	network::FinishServerMessageBuffer(_authFailed, msg);
}

void UserConnectHandler::sendAuthFailed(ENetPeer* peer) {
	ENetPacket* packet = _messageSender->createServerPacket(network::ServerMsgType::AuthFailed, _authFailed.GetBufferPointer(), _authFailed.GetSize(), ENET_PACKET_FLAG_RELIABLE);
	_network->sendMessage(peer, packet);
}

UserPtr UserConnectHandler::login(ENetPeer* peer, const core::String& email, const core::String& passwd) {
	db::UserModel model;
	const db::DBConditionUserModelEmail emailCond(email.c_str());
	const db::DBConditionUserModelPassword passwordCond(passwd.c_str());
	_dbHandler->select(model, persistence::DBConditionMultiple(true, {&emailCond, &passwordCond}));
	if (model.id() == (int64_t)0) {
		Log::warn(logid, "Could not get user id for email: %s", email.c_str());
		return UserPtr();
	}
	const UserPtr& user = _entityStorage->user(model.id());
	if (user) {
		ENetPeer* oldPeer = user->peer();
		if (oldPeer == nullptr || oldPeer->address.host == peer->address.host) {
			Log::debug(logid, "user %i reconnects with host %u on port %i", (int) model.id(), peer->address.host, peer->address.port);
			user->setPeer(peer);
			user->onReconnect();
			return user;
		}
		Log::debug(logid, "skip connection attempt for client %i - the hosts don't match", (int) model.id());
		return UserPtr();
	}
	static const core::String name = "NONAME";
	MapPtr map = _mapProvider->map(model.mapid(), true);
	Log::info(logid, "user %i connects with host %u on port %i", (int) model.id(), peer->address.host, peer->address.port);
	const UserPtr& u = std::make_shared<User>(peer, model.id(), model.name(), map, _messageSender, _timeProvider,
			_containerProvider, _cooldownProvider, _dbHandler, _persistenceMgr, _stockDataProvider);
	u->init();
	map->addUser(u);
	_entityStorage->addUser(u);
	return u;
}

void UserConnectHandler::execute(ENetPeer* peer, const void* raw) {
	const auto* message = getMsg<network::UserConnect>(raw);

	const core::String email(message->email()->c_str());
	if (!util::isValidEmail(email)) {
		sendAuthFailed(peer);
		Log::debug(logid, "Invalid email given: '%s', %c", email.c_str(), email[0]);
		return;
	}
	const core::String password(message->password()->c_str());
	if (password.empty()) {
		Log::debug(logid, "User tries to log into the server without providing a password");
		sendAuthFailed(peer);
		return;
	}
	Log::debug(logid, "User %s tries to log into the server", email.c_str());

	const UserPtr& user = login(peer, email, password);
	if (!user) {
		sendAuthFailed(peer);
		return;
	}

	user->onConnect();
}

}
