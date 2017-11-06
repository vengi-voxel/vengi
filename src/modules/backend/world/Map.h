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
	bool updateEntity(const EntityPtr& entity, long dt);

	glm::vec3 findStartPosition(const EntityPtr& entity) const;

public:
	Map(MapId mapId, const core::EventBusPtr& eventBus);
	~Map();

	void update(long dt);

	bool init(const io::FilesystemPtr& filesystem);
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

	void addNpc(const NpcPtr& npc);
	bool removeNpc(ai::CharacterId id);
	NpcPtr npc(ai::CharacterId id);

	ai::Zone* zone() const;
	MapId id() const;

	int findFloor(const glm::vec3& pos) const;
	glm::ivec3 randomPos() const;
};

inline MapId Map::id() const {
	return _mapId;
}

inline ai::Zone* Map::zone() const {
	return _zone;
}

typedef std::shared_ptr<Map> MapPtr;

}
