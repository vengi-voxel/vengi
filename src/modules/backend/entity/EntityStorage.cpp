/**
 * @file
 */

#include "EntityStorage.h"
#include "core/EventBus.h"
#include "User.h"
#include "Npc.h"
#include "backend/eventbus/Event.h"

namespace backend {

EntityStorage::EntityStorage(const core::EventBusPtr& eventBus) :
		_eventBus(eventBus), _time(0L) {
	_eventBus->subscribe<EntityDeleteEvent>(*this);
}

bool EntityStorage::addUser(const UserPtr& user) {
	auto i = _users.insert(std::make_pair(user->id(), user));
	if (!i.second) {
		return false;
	}
	_eventBus->publish(EntityAddEvent(user));
	return true;
}

bool EntityStorage::removeUser(EntityId userId) {
	auto i = _users.find(userId);
	if (i == _users.end()) {
		return false;
	}
	_users.erase(i);
	i->second->shutdown();
	const uint64_t count = i->second.use_count();
	if (count != 1) {
		Log::warn("Someone is still holding a reference to the user object: %" SDL_PRIu64, count);
	}
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

bool EntityStorage::addNpc(const NpcPtr& npc) {
	auto i = _npcs.insert(std::make_pair(npc->id(), npc));
	if (!i.second) {
		return false;
	}
	_eventBus->publish(EntityAddEvent(npc));
	return true;
}

void EntityStorage::onEvent(const EntityDeleteEvent& event) {
	const EntityId id = event.entityId();
	const network::EntityType type = event.entityType();
	if (type == network::EntityType::PLAYER) {
		if (!removeUser(id)) {
			Log::warn("Could not delete user with id " PRIEntId, id);
		}
	} else {
		if (!removeNpc(id)) {
			Log::warn("Could not delete npc with id " PRIEntId, id);
		}
	}
}

bool EntityStorage::removeNpc(EntityId id) {
	NpcsIter i = _npcs.find(id);
	if (i == _npcs.end()) {
		return false;
	}
	_npcs.erase(i);
	const uint64_t count = i->second.use_count();
	i->second->shutdown();
	if (count != 1) {
		Log::warn("Someone is still holding a reference to the npc object: %" SDL_PRIu64, count);
	}
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
