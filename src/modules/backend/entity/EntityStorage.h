/**
 * @file
 */

#pragma once

#include "EntityId.h"
#include "Npc.h"
#include "network/Network.h"
#include "network/MessageSender.h"
#include "core/QuadTree.h"
#include <unordered_map>

namespace backend {

class User;
typedef std::shared_ptr<User> UserPtr;

class EntityStorage {
private:
	typedef std::unordered_map<EntityId, UserPtr> Users;
	typedef Users::iterator UsersIter;
	Users _users;

	typedef std::unordered_map<ai::CharacterId, NpcPtr> Npcs;
	typedef Npcs::iterator NpcsIter;
	Npcs _npcs;

	/**
	 * this node contains either a user or a npc - but never both
	 * this is used for the quadtree
	 */
	struct Node {
		EntityPtr entity;

		core::RectFloat getRect() const;
		bool operator==(const Node& rhs) const;
	};

	core::QuadTree<Node, float> _quadTree;
	core::QuadTreeCache<Node, float> _quadTreeCache;

	network::MessageSenderPtr _messageSender;
	voxel::WorldPtr _world;
	core::TimeProviderPtr _timeProvider;
	attrib::ContainerProviderPtr _containerProvider;
	PoiProviderPtr _poiProvider;
	long _time;

	void registerUser(const UserPtr& user);
	// users are controlling npcs - and here we update them and send the messages to
	// the users that are seeing this npc entity.
	// users itself are not visible until they have taken over a npc
	bool updateEntity(const EntityPtr& entity, long dt);
	void updateQuadTree();

	EntityId getUserId(const std::string& user, const std::string& passwd) const;
public:
	EntityStorage(network::MessageSenderPtr messageSender, voxel::WorldPtr world, core::TimeProviderPtr timeProvider,
			attrib::ContainerProviderPtr containerProvider, PoiProviderPtr poiProvider);

	UserPtr login(ENetPeer* peer, const std::string& email, const std::string& passwd);
	bool logout(EntityId userId);

	void addNpc(const NpcPtr& npc);
	bool removeNpc(ai::CharacterId id);
	NpcPtr getNpc(ai::CharacterId id);

	void onFrame(long dt);
};

typedef std::shared_ptr<EntityStorage> EntityStoragePtr;

}
