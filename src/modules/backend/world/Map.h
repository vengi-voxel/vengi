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

class Map {
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
public:
	Map(MapId mapId, const core::EventBusPtr& eventBus);
	~Map();

	void update(long dt);

	bool init(const io::FilesystemPtr& filesystem);
	void shutdown();

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
