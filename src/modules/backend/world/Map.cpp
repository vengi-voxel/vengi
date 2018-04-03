/**
 * @file
 */

#include "Map.h"
#include "LUAFunctions.h"
#include "voxel/World.h"
#include "core/String.h"
#include "core/EventBus.h"
#include "core/App.h"
#include "math/QuadTree.h"
#include "io/Filesystem.h"
#include "backend/entity/Npc.h"
#include "backend/entity/User.h"
#include "ai/zone/Zone.h"
#include "metric/MetricEvent.h"
#include "poi/PoiProvider.h"
#include "backend/eventbus/Event.h"
#include "backend/spawn/SpawnMgr.h"

namespace backend {

math::RectFloat Map::QuadTreeNode::getRect() const {
	return entity->rect();
}

bool Map::QuadTreeNode::operator==(const QuadTreeNode& rhs) const {
	return rhs.entity == entity;
}

Map::Map(MapId mapId,
		const core::EventBusPtr& eventBus,
		const core::TimeProviderPtr& timeProvider,
		const io::FilesystemPtr& filesystem,
		const EntityStoragePtr& entityStorage,
		const network::ServerMessageSenderPtr& messageSender,
		const AILoaderPtr& loader,
		const attrib::ContainerProviderPtr& containerProvider,
		const cooldown::CooldownProviderPtr& cooldownProvider) :
		_mapId(mapId), _mapIdStr(std::to_string(mapId)),
		_eventBus(eventBus), _filesystem(filesystem), _attackMgr(this),
		_quadTree(math::RectFloat::getMaxRect(), 100.0f), _quadTreeCache(_quadTree) {
	_poiProvider = std::make_shared<poi::PoiProvider>(timeProvider);
	_spawnMgr = std::make_shared<backend::SpawnMgr>(this, filesystem, entityStorage, messageSender,
			timeProvider, loader, containerProvider, cooldownProvider);
}

Map::~Map() {
	shutdown();
}

bool Map::updateEntity(const EntityPtr& entity, long dt) {
	if (!entity->update(dt)) {
		return false;
	}
	const math::RectFloat& rect = entity->viewRect();
	// TODO: maybe move into the entity instance to reduce memory allocations.
	math::QuadTree<QuadTreeNode, float>::Contents contents;
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
	if (_luaUpdate && !_lua.execute("update")) {
		Log::debug("Could not execute 'update' function for map id %i (%s)", _mapId, _lua.error().c_str());
		_luaUpdate = false;
	}
	_spawnMgr->update(dt);
	_zone->update(dt);
	_attackMgr.update(dt);

	for (auto i = _users.begin(); i != _users.end();) {
		UserPtr user = i->second;
		if (updateEntity(user, dt)) {
			++i;
			continue;
		}
		Log::debug("remove user " PRIEntId, user->id());
		_quadTree.remove(QuadTreeNode { user });
		i = _users.erase(i);
		_eventBus->enqueue(std::make_shared<EntityDeleteEvent>(user->id(), user->entityType()));
	}
	for (auto i = _npcs.begin(); i != _npcs.end();) {
		NpcPtr npc = i->second;
		if (updateEntity(npc, dt)) {
			++i;
			continue;
		}
		Log::debug("remove npc " PRIEntId, npc->id());
		_quadTree.remove(QuadTreeNode { npc });
		i = _npcs.erase(i);
		_zone->removeAI(npc->ai());
		_eventBus->enqueue(std::make_shared<EntityDeleteEvent>(npc->id(), npc->entityType()));
	}
}

bool Map::init() {
	if (!_attackMgr.init()) {
		return false;
	}
	_voxelWorld = new voxel::World();
	const std::string& worldParamData = _filesystem->load("worldparams.lua");
	const std::string& biomesData = _filesystem->load("biomes.lua");
	if (!_voxelWorld->init(worldParamData, biomesData)) {
		Log::error("Failed to init map with id %i", _mapId);
		return false;
	}
	const core::VarPtr& seed = core::Var::getSafe(cfg::ServerSeed);
	_voxelWorld->setSeed(seed->longVal());
	_voxelWorld->setPersist(false);
	_zone = new ai::Zone(core::string::format("Zone %i", _mapId));

	if (!_spawnMgr->init()) {
		Log::error("Failed to init the spawn manager");
		return false;
	}

	const std::string& mapData = _filesystem->load("map/map%03i.lua", _mapId);
	if (mapData.empty()) {
		return true;
	}

	lua::LUAType map = _lua.registerType("Map");
	map.addFunction("id", luaMapGetId);
	map.addFunction("__gc", luaMapGC);
	map.addFunction("__tostring", luaMapToString);

	_lua.registerGlobal("map", luaGetMap);

	if (!_lua.load(mapData)) {
		Log::warn("Could not load map data for map id %i (%s)", _mapId, _lua.error().c_str());
		return false;
	}

	if (!_lua.execute("init")) {
		Log::debug("Could not execute 'init' function for map id %i (%s)", _mapId, _lua.error().c_str());
	}

	return true;
}

void Map::shutdown() {
	if (!_lua.execute("shutdown")) {
		Log::debug("Could not execute 'shutdown' function for map id %i (%s)", _mapId, _lua.error().c_str());
	}
	_attackMgr.shutdown();
	_spawnMgr->shutdown();
	if (_voxelWorld != nullptr) {
		_voxelWorld->shutdown();
		delete _voxelWorld;
		_voxelWorld = nullptr;
	}
	delete _zone;
	_zone = nullptr;
}

glm::vec3 Map::findStartPosition(const EntityPtr& entity) const {
	const poi::PoiResult& result = _poiProvider->query(poi::Type::GENERIC);
	if (result.valid) {
		return result.pos;
	}
	// TODO:
	return glm::zero<glm::vec3>();
}

void Map::addUser(const UserPtr& user) {
	auto i = _users.insert(std::make_pair(user->id(), user));
	if (!i.second) {
		return;
	}
	const glm::vec3& pos = findStartPosition(user);
	user->setMap(ptr(), pos);
	_quadTree.insert(QuadTreeNode { user });
	_eventBus->enqueue(std::make_shared<EntityAddToMapEvent>(user));
	_poiProvider->add(pos, poi::Type::SPAWN);
}

bool Map::removeUser(EntityId id) {
	UsersIter i = _users.find(id);
	if (i == _users.end()) {
		return false;
	}
	UserPtr user = i->second;
	_quadTree.remove(QuadTreeNode { user });
	_users.erase(i);
	_eventBus->enqueue(std::make_shared<EntityRemoveFromMapEvent>(user));
	return true;
}

UserPtr Map::user(EntityId id) {
	UsersIter i = _users.find(id);
	if (i == _users.end()) {
		Log::trace("Could not find user with id " PRIEntId, id);
		return UserPtr();
	}
	return i->second;
}

bool Map::addNpc(const NpcPtr& npc) {
	auto i = _npcs.insert(std::make_pair(npc->id(), npc));
	if (!i.second) {
		return false;
	}
	const glm::vec3& pos = findStartPosition(npc);
	npc->setMap(ptr(), pos);
	_zone->addAI(npc->ai());
	_quadTree.insert(QuadTreeNode { npc });
	_eventBus->enqueue(std::make_shared<EntityAddToMapEvent>(npc));
	_poiProvider->add(pos, poi::Type::SPAWN);
	return true;
}

bool Map::removeNpc(EntityId id) {
	NpcsIter i = _npcs.find(id);
	if (i == _npcs.end()) {
		return false;
	}
	NpcPtr npc = i->second;
	_quadTree.remove(QuadTreeNode { npc });
	_npcs.erase(i);
	_zone->removeAI(npc->ai());
	_eventBus->enqueue(std::make_shared<EntityRemoveFromMapEvent>(npc));
	return true;
}

NpcPtr Map::npc(EntityId id) {
	NpcsIter i = _npcs.find(id);
	if (i == _npcs.end()) {
		Log::trace("Could not find npc with id " PRIEntId, id);
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

}
