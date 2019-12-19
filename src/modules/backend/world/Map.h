/**
 * @file
 */

#pragma once

#include "backend/ForwardDecl.h"
#include "math/QuadTree.h"
#include "math/Rect.h"
#include "core/Common.h"
#include "ai/common/CharacterId.h"
#include "core/IComponent.h"
#include "backend/attack/AttackMgr.h"
#include "persistence/ISavable.h"
#include "persistence/ForwardDecl.h"
#include "voxel/Constants.h"
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
	std::string _mapIdStr;
	voxelworld::WorldMgr* _voxelWorldMgr = nullptr;

	core::EventBusPtr _eventBus;
	SpawnMgrPtr _spawnMgr;
	poi::PoiProviderPtr _poiProvider;
	io::FilesystemPtr _filesystem;
	persistence::PersistenceMgrPtr _persistenceMgr;
	voxelformat::VolumeCachePtr _volumeCache;

	ai::Zone* _zone = nullptr;

	typedef std::unordered_map<ai::CharacterId, NpcPtr> Npcs;
	typedef Npcs::iterator NpcsIter;
	Npcs _npcs;

	typedef std::unordered_map<EntityId, UserPtr> Users;
	typedef Users::iterator UsersIter;
	Users _users;

	AttackMgr _attackMgr;

	struct QuadTreeNode {
		EntityPtr entity;

		math::RectFloat getRect() const;
		bool operator==(const QuadTreeNode& rhs) const;
	};

	math::QuadTree<QuadTreeNode, float> _quadTree;

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
			const voxelformat:: VolumeCachePtr& volumeCache,
			const AILoaderPtr& loader,
			const attrib::ContainerProviderPtr& containerProvider,
			const cooldown::CooldownProviderPtr& cooldownProvider,
			const persistence::PersistenceMgrPtr& persistenceMgr);
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

	ai::Zone* zone() const;
	MapId id() const;
	const std::string& idStr() const;

	int npcCount() const;
	int userCount() const;

	int findFloor(const glm::vec3& pos, float maxDistanceY = (float)voxel::MAX_HEIGHT) const;
	glm::ivec3 randomPos() const;

	const AttackMgr& attackMgr() const;
	AttackMgr& attackMgr();

	const SpawnMgrPtr& spawnMgr() const;
	SpawnMgrPtr& spawnMgr();

	const poi::PoiProviderPtr& poiProvider() const;
	poi::PoiProviderPtr& poiProvider();
};

inline const AttackMgr& Map::attackMgr() const {
	return _attackMgr;
}

inline AttackMgr& Map::attackMgr() {
	return _attackMgr;
}

inline MapId Map::id() const {
	return _mapId;
}

inline const std::string& Map::idStr() const {
	return _mapIdStr;
}

inline const SpawnMgrPtr& Map::spawnMgr() const {
	return _spawnMgr;
}

inline SpawnMgrPtr& Map::spawnMgr() {
	return _spawnMgr;
}

inline const poi::PoiProviderPtr& Map::poiProvider() const {
	return _poiProvider;
}

inline poi::PoiProviderPtr& Map::poiProvider() {
	return _poiProvider;
}

inline ai::Zone* Map::zone() const {
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
