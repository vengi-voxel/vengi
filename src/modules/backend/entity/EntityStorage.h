/**
 * @file
 */

#pragma once

#include "Entity.h"
#include "Npc.h"
#include "network/Network.h"
#include "core/QuadTree.h"
#include "core/TimeProvider.h"
#include <unordered_map>

namespace backend {

class User;
typedef std::shared_ptr<User> UserPtr;

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

	network::MessageSenderPtr _messageSender;
	voxel::WorldPtr _world;
	core::TimeProviderPtr _timeProvider;
	attrib::ContainerProviderPtr _containerProvider;
	PoiProviderPtr _poiProvider;
	cooldown::CooldownDurationPtr _cooldownDuration;
	long _time;

	void registerUser(const UserPtr& user);
	// users are controlling npcs - and here we update them and send the messages to
	// the users that are seeing this npc entity.
	// users itself are not visible until they have taken over a npc
	bool updateEntity(const EntityPtr& entity, long dt);
	void updateQuadTree();

	EntityId getUserId(const std::string& email, const std::string& password) const;
public:
	EntityStorage(const network::MessageSenderPtr& messageSender, const voxel::WorldPtr& world, const core::TimeProviderPtr& timeProvider,
			const attrib::ContainerProviderPtr& containerProvider, const PoiProviderPtr& poiProvider, const cooldown::CooldownDurationPtr& cooldownDuration);

	UserPtr login(ENetPeer* peer, const std::string& email, const std::string& password);
	bool logout(EntityId userId);

	void addNpc(const NpcPtr& npc);
	bool removeNpc(ai::CharacterId id);
	NpcPtr getNpc(ai::CharacterId id);

	void onFrame(long dt);
};

typedef std::shared_ptr<EntityStorage> EntityStoragePtr;

}
