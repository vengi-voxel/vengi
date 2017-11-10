/**
 * @file
 */

#pragma once

#include "backend/ForwardDecl.h"
#include "commonlua/LUA.h"
#include "core/QuadTree.h"
#include "core/Rect.h"
#include "ai/common/Types.h"
#include "MapId.h"
#include <memory>
#include <unordered_map>
#include <glm/vec3.hpp>

namespace backend {

class Map : public std::enable_shared_from_this<Map> {
private:
	MapId _mapId;
	std::string _mapIdStr;
	voxel::World* _voxelWorld = nullptr;

	core::EventBusPtr _eventBus;
	SpawnMgrPtr _spawnMgr;
	poi::PoiProviderPtr _poiProvider;
	io::FilesystemPtr _filesystem;

	lua::LUA _lua;

	ai::Zone* _zone = nullptr;

	typedef std::unordered_map<ai::CharacterId, NpcPtr> Npcs;
	typedef Npcs::iterator NpcsIter;
	Npcs _npcs;

	typedef std::unordered_map<EntityId, UserPtr> Users;
	typedef Users::iterator UsersIter;
	Users _users;

	struct QuadTreeNode {
		EntityPtr entity;

		core::RectFloat getRect() const;
		bool operator==(const QuadTreeNode& rhs) const;
	};

	core::QuadTree<QuadTreeNode, float> _quadTree;
	core::QuadTreeCache<QuadTreeNode, float> _quadTreeCache;

	void updateQuadTree();
	/**
	 * @return @c false if the entity should be removed from the server.
	 */
	bool updateEntity(const EntityPtr& entity, long dt);

	glm::vec3 findStartPosition(const EntityPtr& entity) const;

public:
	Map(MapId mapId,
			const core::EventBusPtr& eventBus,
			const core::TimeProviderPtr& timeProvider,
			const io::FilesystemPtr& filesystem,
			const EntityStoragePtr& entityStorage,
			const network::ServerMessageSenderPtr& messageSender,
			const AILoaderPtr& loader,
			const attrib::ContainerProviderPtr& containerProvider,
			const cooldown::CooldownProviderPtr& cooldownProvider);
	~Map();

	void update(long dt);

	bool init();
	void shutdown();

	/**
	 * If the object is currently maintained by a shared_ptr, you can get a shared_ptr from a raw pointer
	 * instance that shares the state with the already existing shared_ptrs around.
	 */
	inline std::shared_ptr<Map> ptr() {
		return shared_from_this();
	}

	/**
	 * @brief Spawns a user at this map - also sets a suitable position
	 * @note Updates the map instance of the @c User
	 */
	void addUser(const UserPtr& user);
	/**
	 * @note The user will keep this map set up to the point a new @c addUser() was called on another map instance.
	 */
	bool removeUser(EntityId id);
	UserPtr user(EntityId id);

	void addNpc(const NpcPtr& npc);
	bool removeNpc(EntityId id);
	NpcPtr npc(EntityId id);

	ai::Zone* zone() const;
	MapId id() const;
	const std::string& idStr() const;

	int findFloor(const glm::vec3& pos) const;
	glm::ivec3 randomPos() const;

	const SpawnMgrPtr& spawnMgr() const;
	const poi::PoiProviderPtr& poiProvider() const;
};

inline MapId Map::id() const {
	return _mapId;
}

inline const std::string& Map::idStr() const {
	return _mapIdStr;
}

inline const SpawnMgrPtr& Map::spawnMgr() const {
	return _spawnMgr;
}

inline const poi::PoiProviderPtr& Map::poiProvider() const {
	return _poiProvider;
}

inline ai::Zone* Map::zone() const {
	return _zone;
}

typedef std::shared_ptr<Map> MapPtr;

}
