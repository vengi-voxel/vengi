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

EntityStorage::~EntityStorage() {
	core_assert(_npcs.empty());
	core_assert(_users.empty());
}

bool EntityStorage::addUser(const UserPtr& user) {
	auto i = _users.insert(std::make_pair(user->id(), user));
	if (!i.second) {
		Log::debug("User with id " PRIEntId " is already connected", user->id());
		return false;
	}
	Log::info("User with id " PRIEntId " is connected", user->id());
	_eventBus->publish(EntityAddEvent(user));
	return true;
}

bool EntityStorage::removeUser(EntityId userId) {
	auto i = _users.find(userId);
	if (i == _users.end()) {
		Log::warn("User with id " PRIEntId " can't get removed. Reason: NotFound", userId);
		return false;
	}
	Log::warn("User with id " PRIEntId " is going to be removed", userId);
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
		Log::warn("Could not add npc with id " PRIEntId ". Reason: AlreadyExists", npc->id());
		return false;
	}
	Log::debug("Add npc with id " PRIEntId, npc->id());
	_eventBus->publish(EntityAddEvent(npc));
	return true;
}

void EntityStorage::onEvent(const EntityDeleteEvent& event) {
	const EntityId id = event.entityId();
	const network::EntityType type = event.entityType();
	if (type == network::EntityType::PLAYER) {
		removeUser(id);
	} else {
		removeNpc(id);
	}
}

bool EntityStorage::removeNpc(EntityId id) {
	NpcsIter i = _npcs.find(id);
	if (i == _npcs.end()) {
		Log::warn("Could not delete npc with id " PRIEntId, id);
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
