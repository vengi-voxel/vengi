/**
 * @file
 */

#pragma once

#include "backend/ForwardDecl.h"
#include "math/QuadTree.h"
#include "math/Rect.h"
#include "core/Common.h"
#include "core/FourCC.h"
#include "ai-shared/common/CharacterId.h"
#include "voxelutil/FloorTraceResult.h"
#include "core/IComponent.h"
#include "backend/attack/AttackMgr.h"
#include "persistence/ISavable.h"
#include "persistence/ForwardDecl.h"
#include "poi/PoiProvider.h"
#include "backend/spawn/SpawnMgr.h"
#include "voxel/Constants.h"
#include "DBChunkPersister.h"
#include "MapId.h"
#include <memory>
#include <unordered_map>
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>

namespace backend {

/**
 * @brief A map contains the Entity instances. This is where the players are moving and npcs are living.
 */
class Map : public std::enable_shared_from_this<Map>, public core::IComponent, public persistence::ISavable {
private:
	static constexpr uint32_t FOURCC = FourCC('M', 'A', 'P', '\0');
	MapId _mapId;
	core::String _mapIdStr;
	voxelworld::WorldMgr* _voxelWorldMgr = nullptr;
	voxelworld::WorldPagerPtr _pager;

	core::EventBusPtr _eventBus;
	io::FilesystemPtr _filesystem;
	persistence::PersistenceMgrPtr _persistenceMgr;
	voxelformat::VolumeCachePtr _volumeCache;

	Zone* _zone = nullptr;

	typedef std::unordered_map<ai::CharacterId, NpcPtr> Npcs;
	typedef Npcs::iterator NpcsIter;
	Npcs _npcs;

	typedef std::unordered_map<EntityId, UserPtr> Users;
	typedef Users::iterator UsersIter;
	Users _users;

	AttackMgr _attackMgr;
	poi::PoiProvider _poiProvider;
	SpawnMgr _spawnMgr;

	struct QuadTreeNode {
		EntityPtr entity;

		math::RectFloat getRect() const;
		bool operator==(const QuadTreeNode& rhs) const;
	};

	math::QuadTree<QuadTreeNode, float> _quadTree;
	DBChunkPersisterPtr _chunkPersister;
	/**
	 * @return @c false if the entity should be removed from the server.
	 */
	bool updateEntity(const EntityPtr& entity, long dt);

	glm::vec3 findStartPosition(const EntityPtr& entity, poi::Type type = poi::Type::GENERIC) const;

public:
	Map(MapId mapId,
			const core::EventBusPtr& eventBus,
			const core::TimeProviderPtr& timeProvider,
			const io::FilesystemPtr& filesystem,
			const EntityStoragePtr& entityStorage,
			const network::ServerMessageSenderPtr& messageSender,
			const voxelformat:: VolumeCachePtr& volumeCache,
			const AILoaderPtr& loader,
			const attrib::ContainerProviderPtr& containerProvider,
			const cooldown::CooldownProviderPtr& cooldownProvider,
			const persistence::PersistenceMgrPtr& persistenceMgr,
			const DBChunkPersisterPtr& chunkPersister);
	~Map();

	void update(long dt);

	bool init() override;
	void shutdown() override;

	bool getDirtyModels(Models& models) override;

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

	bool addNpc(const NpcPtr& npc);
	/**
	 * @brief Remove npc from map but keep it in the world
	 * @note The npc will keep this map set up to the point a new @c addNpc() was called on another map instance.
	 */
	bool removeNpc(EntityId id);
	NpcPtr npc(EntityId id);

	const voxelworld::WorldPagerPtr& pager() const;
	voxelworld::WorldMgr* worldMgr();
	Zone* zone() const;
	MapId id() const;
	const core::String& idStr() const;

	int npcCount() const;
	int userCount() const;

	voxelutil::FloorTraceResult findFloor(const glm::ivec3& pos, int maxDistanceY = voxel::MAX_HEIGHT) const;
	glm::ivec3 randomPos() const;

	const DBChunkPersisterPtr& chunkPersister();

	const AttackMgr& attackMgr() const;
	AttackMgr& attackMgr();

	const SpawnMgr& spawnMgr() const;
	SpawnMgr& spawnMgr();

	const poi::PoiProvider& poiProvider() const;
	poi::PoiProvider& poiProvider();
};

inline const DBChunkPersisterPtr& Map::chunkPersister() {
	return _chunkPersister;
}

inline const voxelworld::WorldPagerPtr& Map::pager() const {
	return _pager;
}

inline voxelworld::WorldMgr* Map::worldMgr() {
	return _voxelWorldMgr;
}

inline const AttackMgr& Map::attackMgr() const {
	return _attackMgr;
}

inline AttackMgr& Map::attackMgr() {
	return _attackMgr;
}

inline MapId Map::id() const {
	return _mapId;
}

inline const core::String& Map::idStr() const {
	return _mapIdStr;
}

inline const SpawnMgr& Map::spawnMgr() const {
	return _spawnMgr;
}

inline SpawnMgr& Map::spawnMgr() {
	return _spawnMgr;
}

inline const poi::PoiProvider& Map::poiProvider() const {
	return _poiProvider;
}

inline poi::PoiProvider& Map::poiProvider() {
	return _poiProvider;
}

inline Zone* Map::zone() const {
	return _zone;
}

inline int Map::npcCount() const {
	return (int)_npcs.size();
}

inline int Map::userCount() const {
	return (int)_users.size();
}

typedef std::shared_ptr<Map> MapPtr;

}
