/**
 * @file
 */

#pragma once

#include "backend/ForwardDecl.h"
#include "ai/common/CharacterId.h"
#include "core/EventBus.h"
#include "backend/eventbus/Event.h"
#include <functional>
#include <unordered_map>

namespace backend {

/**
 * @brief Manages the Entity instances of the backend.
 *
 * This includes calling the Entity::update() method as well as performing the visibility calculations.
 */
class EntityStorage : public core::IEventBusHandler<EntityDeleteEvent>{
private:
	typedef std::unordered_map<EntityId, UserPtr> Users;
	typedef Users::iterator UsersIter;
	Users _users;

	typedef std::unordered_map<EntityId, NpcPtr> Npcs;
	typedef Npcs::iterator NpcsIter;
	Npcs _npcs;

	core::EventBusPtr _eventBus;
public:
	EntityStorage(const core::EventBusPtr& eventBus);
	virtual ~EntityStorage();

	void shutdown();
	bool init();

	void onEvent(const EntityDeleteEvent& event) override;

	bool addUser(const UserPtr& user);
	bool removeUser(EntityId userId);
	UserPtr user(EntityId userId);

	bool addNpc(const NpcPtr& npc);
	bool removeNpc(EntityId id);
	NpcPtr npc(EntityId id);

	void visit(const std::function<void(const EntityPtr&)>& visitor);
	void visitNpcs(const std::function<void(const NpcPtr&)>& visitor);
	void visitUsers(const std::function<void(const UserPtr&)>& visitor);
};

typedef std::shared_ptr<EntityStorage> EntityStoragePtr;

}
