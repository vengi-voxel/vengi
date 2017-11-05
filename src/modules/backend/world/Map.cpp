/**
 * @file
 */

#include "Map.h"
#include "voxel/World.h"
#include "core/String.h"
#include "core/EventBus.h"
#include "core/App.h"
#include "io/Filesystem.h"
#include "backend/entity/Npc.h"
#include "backend/entity/User.h"
#include "ai/zone/Zone.h"
#include "metric/MetricEvent.h"
#include "backend/eventbus/Event.h"

namespace backend {

core::RectFloat Map::QuadTreeNode::getRect() const {
	return entity->rect();
}

bool Map::QuadTreeNode::operator==(const QuadTreeNode& rhs) const {
	return rhs.entity == entity;
}

Map::Map(MapId mapId, const core::EventBusPtr& eventBus) :
		_mapId(mapId), _mapIdStr(std::to_string(mapId)), _quadTree(core::RectFloat::getMaxRect(), 100.0f), _quadTreeCache(_quadTree) {
}

Map::~Map() {
	shutdown();
}

bool Map::updateEntity(const EntityPtr& entity, long dt) {
	if (!entity->update(dt)) {
		return false;
	}
	const core::RectFloat& rect = entity->viewRect();
	// TODO: maybe move into the entity instance to reduce memory allocations.
	core::QuadTree<QuadTreeNode, float>::Contents contents;
	contents.reserve(entity->visibleCount() + 10);
	_quadTreeCache.query(rect, contents);
	EntitySet set;
	set.reserve(contents.size());
	for (const QuadTreeNode& node : contents) {
		// TODO: check the distance - the rect might contain more than the circle would...
		if (entity->inFrustum(node.entity)) {
			set.insert(node.entity);
		}
	}
	set.erase(entity);
	entity->updateVisible(set);
	return true;
}

void Map::update(long dt) {
	_voxelWorld->update(dt);
	_zone->update(dt);
	updateQuadTree();

	for (auto i = _users.begin(); i != _users.end();) {
		UserPtr user = i->second;
		if (!updateEntity(user, dt)) {
			Log::debug("remove user %li", user->id());
			_quadTree.remove(QuadTreeNode { user });
			i = _users.erase(i);
			_eventBus->publish(EntityRemoveEvent(user));
		} else {
			++i;
		}
	}
	for (auto i = _npcs.begin(); i != _npcs.end();) {
		NpcPtr npc = i->second;
		if (!updateEntity(npc, dt)) {
			Log::debug("remove npc %li", npc->id());
			_quadTree.remove(QuadTreeNode { npc });
			i = _npcs.erase(i);
			_eventBus->publish(EntityRemoveEvent(npc));
		} else {
			++i;
		}
	}
}

bool Map::init(const io::FilesystemPtr& filesystem) {
	_voxelWorld = new voxel::World();
	const std::string& worldParamData = filesystem->load("worldparams.lua");
	const std::string& biomesData = filesystem->load("biomes.lua");
	if (!_voxelWorld->init(worldParamData, biomesData)) {
		Log::error("Failed to init map with id %i", _mapId);
		return false;
	}
	const core::VarPtr& seed = core::Var::getSafe(cfg::ServerSeed);
	_voxelWorld->setSeed(seed->longVal());
	_voxelWorld->setPersist(false);
	_zone = new ai::Zone(core::string::format("Zone %i", _mapId));
	const std::string& mapData = filesystem->load(core::string::format("map/map%03i.lua", _mapId));
	if (mapData.empty()) {
		return true;
	}
	if (!_lua.load(mapData)) {
		return false;
	}
	return true;
}

void Map::shutdown() {
	if (_voxelWorld != nullptr) {
		_voxelWorld->shutdown();
		delete _voxelWorld;
		_voxelWorld = nullptr;
	}
	delete _zone;
	_zone = nullptr;
}

void Map::addNpc(const NpcPtr& npc) {
	_zone->addAI(npc->ai());
	_npcs.insert(std::make_pair(npc->id(), npc));
}

bool Map::removeNpc(ai::CharacterId id) {
	NpcsIter i = _npcs.find(id);
	if (i == _npcs.end()) {
		return false;
	}
	NpcPtr npc = i->second;
	_quadTree.remove(QuadTreeNode { npc });
	_npcs.erase(i);
	_eventBus->publish(EntityRemoveEvent(npc));
	return true;
}

NpcPtr Map::npc(ai::CharacterId id) {
	NpcsIter i = _npcs.find(id);
	if (i == _npcs.end()) {
		Log::trace("Could not find npc with id %i", id);
		return NpcPtr();
	}
	return i->second;
}

int Map::findFloor(const glm::vec3& pos) const {
	// the voxel above us shouldn't be solid
	if (!voxel::isFloor(_voxelWorld->material(pos.x, pos.y + 1, pos.z))) {
		if (voxel::isFloor(_voxelWorld->material(pos.x, pos.y, pos.z))) {
			return pos.y;
		}
	}
	return _voxelWorld->findFloor(pos.x, pos.z, voxel::isFloor);
}

glm::ivec3 Map::randomPos() const {
	return _voxelWorld->randomPos();
}

void Map::updateQuadTree() {
	// TODO: a full rebuild is not needed every frame
	_quadTree.clear();
	for (auto i : _npcs) {
		_quadTree.insert(QuadTreeNode { i.second });
	}
	for (auto i : _users) {
		const UserPtr& user = i.second;
		_quadTree.insert(QuadTreeNode { user });
	}
}

}
