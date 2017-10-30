/**
 * @file
 */

#pragma once

#include "backend/ForwardDecl.h"
#include "network/Network.h"
#include "core/QuadTree.h"
#include "core/TimeProvider.h"
#include "ai/common/Types.h"
#include <unordered_map>

namespace backend {

/**
 * @brief Manages the Entity instances of the backend.
 *
 * This includes calling the Entity::update() method as well as performing the visibility calculations.
 */
class EntityStorage {
private:
	typedef std::unordered_map<EntityId, UserPtr> Users;
	typedef Users::iterator UsersIter;
	Users _users;

	typedef std::unordered_map<ai::CharacterId, NpcPtr> Npcs;
	typedef Npcs::iterator NpcsIter;
	Npcs _npcs;

	struct QuadTreeNode {
		EntityPtr entity;

		core::RectFloat getRect() const;
		bool operator==(const QuadTreeNode& rhs) const;
	};

	core::QuadTree<QuadTreeNode, float> _quadTree;
	core::QuadTreeCache<QuadTreeNode, float> _quadTreeCache;

	network::ServerMessageSenderPtr _messageSender;
	voxel::WorldPtr _world;
	core::TimeProviderPtr _timeProvider;
	attrib::ContainerProviderPtr _containerProvider;
	poi::PoiProviderPtr _poiProvider;
	cooldown::CooldownProviderPtr _cooldownProvider;
	persistence::DBHandlerPtr _dbHandler;
	stock::StockProviderPtr _stockDataProvider;
	core::EventBusPtr _eventBus;
	long _time;

	void registerUser(const UserPtr& user);
	bool updateEntity(const EntityPtr& entity, long dt);
	void updateQuadTree();

	EntityId getUserId(const std::string& email, const std::string& password) const;
public:
	EntityStorage(const network::ServerMessageSenderPtr& messageSender, const voxel::WorldPtr& world, const core::TimeProviderPtr& timeProvider,
			const attrib::ContainerProviderPtr& containerProvider, const poi::PoiProviderPtr& poiProvider, const cooldown::CooldownProviderPtr& cooldownProvider,
			const persistence::DBHandlerPtr& dbHandler, const stock::StockProviderPtr& stockDataProvider, const core::EventBusPtr& eventBus);

	UserPtr login(ENetPeer* peer, const std::string& email, const std::string& password);
	bool logout(EntityId userId);

	void addNpc(const NpcPtr& npc);
	bool removeNpc(ai::CharacterId id);
	NpcPtr getNpc(ai::CharacterId id);

	void update(long dt);
};

typedef std::shared_ptr<EntityStorage> EntityStoragePtr;

}
