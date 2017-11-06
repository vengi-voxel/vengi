/**
 * @file
 */

#include "EntityStorage.h"
#include "core/Var.h"
#include "core/EventBus.h"
#include "User.h"
#include "Npc.h"
#include "persistence/DBHandler.h"
#include "stock/StockDataProvider.h"
#include "backend/eventbus/Event.h"

namespace backend {

EntityStorage::EntityStorage(const core::EventBusPtr& eventBus) :
		_eventBus(eventBus), _time(0L) {
}

void EntityStorage::addUser(const UserPtr& user) {
	_users[user->id()] = user;
}

bool EntityStorage::removeUser(EntityId userId) {
	auto i = _users.find(userId);
	if (i == _users.end()) {
		return false;
	}
	_eventBus->publish(EntityDeleteEvent(i->second));
	_users.erase(i);
	return true;
}

UserPtr EntityStorage::user(EntityId id) {
	UsersIter i = _users.find(id);
	if (i == _users.end()) {
		Log::trace("Could not find user with id " PRIEntId, id);
		return UserPtr();
	}
	return i->second;
}

void EntityStorage::addNpc(const NpcPtr& npc) {
	_npcs.insert(std::make_pair(npc->id(), npc));
	_eventBus->publish(EntityAddEvent(npc));
}

bool EntityStorage::removeNpc(EntityId id) {
	NpcsIter i = _npcs.find(id);
	if (i == _npcs.end()) {
		return false;
	}
	_eventBus->publish(EntityDeleteEvent(i->second));
	_npcs.erase(i);
	return true;
}

NpcPtr EntityStorage::npc(EntityId id) {
	NpcsIter i = _npcs.find(id);
	if (i == _npcs.end()) {
		Log::trace("Could not find npc with id " PRIEntId, id);
		return NpcPtr();
	}
	return i->second;
}

}
