/**
 * @file
 */

#include "EntityStorage.h"
#include "core/Var.h"
#include "core/Password.h"
#include "core/EventBus.h"
#include "User.h"
#include "Npc.h"
#include "persistence/DBHandler.h"
#include "stock/StockDataProvider.h"
#include "backend/world/MapProvider.h"
#include "metric/MetricEvent.h"

#define broadcastMsg(msg, type) _messageSender->broadcastServerMessage(fbb, network::type, network::msg.Union());

namespace backend {

EntityStorage::EntityStorage(const MapProviderPtr& mapProvider, const network::ServerMessageSenderPtr& messageSender,
		const core::TimeProviderPtr& timeProvider, const attrib::ContainerProviderPtr& containerProvider,
		const poi::PoiProviderPtr& poiProvider, const cooldown::CooldownProviderPtr& cooldownProvider,
		const persistence::DBHandlerPtr& dbHandler, const stock::StockProviderPtr& stockDataProvider,
		const core::EventBusPtr& eventBus) :
		_messageSender(messageSender),
		_timeProvider(timeProvider), _containerProvider(containerProvider), _poiProvider(poiProvider),
		_cooldownProvider(cooldownProvider), _dbHandler(dbHandler), _stockDataProvider(stockDataProvider),
		_eventBus(eventBus), _mapProvider(mapProvider), _time(0L) {
}

void EntityStorage::registerUser(const UserPtr& user) {
	_users[user->id()] = user;
}

bool EntityStorage::userData(db::UserModel& model, const std::string& email, const std::string& password) const {
	if (!_dbHandler->select(model, db::DBConditionUserEmail(email.c_str()))) {
		return false;
	}
	return password == core::pwhash(model.password());
}

UserPtr EntityStorage::login(ENetPeer* peer, const std::string& email, const std::string& passwd) {
	db::UserModel model;
	if (!userData(model, email, passwd)) {
		Log::warn("Could not get user id for email: %s", email.c_str());
		return UserPtr();
	}
	auto i = _users.find(model.id());
	if (i == _users.end()) {
		static const std::string name = "NONAME";
		const MapPtr& map = _mapProvider->map(model.mapid());
		Log::info("user %i connects with host %i on port %i", (int) model.id(), peer->address.host, peer->address.port);
		const UserPtr& u = std::make_shared<User>(peer, model.id(), model.name(), map, _messageSender, _timeProvider,
				_containerProvider, _cooldownProvider, _poiProvider, _dbHandler, _stockDataProvider);
		u->init();
		registerUser(u);
		return u;
	}
	const UserPtr& u = i->second;
	if (u->host() == peer->address.host) {
		Log::info("user %i reconnects with host %i on port %i", (int) model.id(), peer->address.host, peer->address.port);
		i->second->setPeer(peer);
		i->second->reconnect();
		return i->second;
	}

	Log::info("skip connection attempt for client %i - the hosts don't match", (int) model.id());
	return UserPtr();
}

bool EntityStorage::logout(EntityId userId) {
	auto i = _users.find(userId);
	if (i == _users.end()) {
		return false;
	}
	_users.erase(i);
	return true;
}

void EntityStorage::addNpc(const NpcPtr& npc) {
	_npcs.insert(std::make_pair(npc->id(), npc));
	_eventBus->publish(metric::gauge("count.npc", _npcs.size()));
}

bool EntityStorage::removeNpc(ai::CharacterId id) {
	NpcsIter i = _npcs.find(id);
	if (i == _npcs.end()) {
		return false;
	}
	_npcs.erase(id);
	_eventBus->publish(metric::gauge("count.npc", _npcs.size()));
	return true;
}

NpcPtr EntityStorage::npc(ai::CharacterId id) {
	NpcsIter i = _npcs.find(id);
	if (i == _npcs.end()) {
		Log::trace("Could not find npc with id %i", id);
		return NpcPtr();
	}
	return i->second;
}

void EntityStorage::update(long dt) {
}

}
