/**
 * @file
 */

#pragma once

#include "core/GLM.h"
#include <unordered_set>
#include <memory>
#include "core/Rect.h"
#include "core/ReadWriteLock.h"
#include "attrib/Attributes.h"
#include "attrib/ContainerProvider.h"
#include "cooldown/CooldownMgr.h"
#include "network/ServerMessageSender.h"
#include "EntityId.h"
#include "backend/ForwardDecl.h"

namespace voxel {
class World;
typedef std::shared_ptr<World> WorldPtr;
}

namespace backend {

class Entity;
typedef std::shared_ptr<Entity> EntityPtr;
typedef std::unordered_set<EntityPtr> EntitySet;

/**
 * @brief Every actor in the world is an entity
 *
 * Entities are updated via @c network::ServerMsgType::EntityUpdate
 * message for the clients that are seeing the entity
 *
 * @sa EntityUpdateHandler
 */
class Entity : public std::enable_shared_from_this<Entity> {
private:
	core::ReadWriteLock _visibleLock {"Entity"};
	EntitySet _visible;

protected:
	// network stuff
	network::ServerMessageSenderPtr _messageSender;
	ENetPeer *_peer = nullptr;

	// attribute stuff
	attrib::ContainerProviderPtr _containerProvider;
	attrib::Attributes _attribs;
	std::unordered_set<attrib::DirtyValue> _dirtyAttributeTypes;

	// cooldowns
	cooldown::CooldownMgr _cooldowns;

	MapPtr _map;

	EntityId _entityId;
	network::EntityType _entityType = network::EntityType::NONE;
	glm::vec3 _pos;
	float _orientation = 0.0f;
	float _size = 1.0f;

	/**
	 * @brief Called with the set of entities that just get visible for this entity
	 */
	virtual void visibleAdd(const EntitySet& entities);
	/**
	 * @brief Called with the set of entities that just get invisible for this entity
	 */
	virtual void visibleRemove(const EntitySet& entities);

	void sendAttribUpdate();
	void sendEntityUpdate(const EntityPtr& entity) const;
	void sendEntitySpawn(const EntityPtr& entity) const;
	void sendEntityRemove(const EntityPtr& entity) const;

	void onAttribChange(const attrib::DirtyValue& v);
public:
	Entity(EntityId id,
			const MapPtr& map,
			const network::ServerMessageSenderPtr& messageSender,
			const core::TimeProviderPtr& timeProvider,
			const attrib::ContainerProviderPtr& containerProvider,
			const cooldown::CooldownProviderPtr& cooldownProvider);
	virtual ~Entity();

	void addContainer(const std::string& id);
	void removeContainer(const std::string& id);

	cooldown::CooldownMgr& cooldownMgr();

	EntityId id() const;
	MapPtr map() const;
	void setMap(const MapPtr& map, const glm::vec3& pos);

	bool dead() const;

	ENetPeer* peer() const;

	/**
	 * If the object is currently maintained by a shared_ptr, you can get a shared_ptr from a raw pointer
	 * instance that shares the state with the already existing shared_ptrs around.
	 */
	inline std::shared_ptr<Entity> ptr() {
		return shared_from_this();
	}

	/**
	 * @note The implementation behind this must ensure thread safety
	 * @return the current position in world coordinates
	 */
	glm::vec3 pos() const;
	void setPos(const glm::vec3& pos);

	float orientation() const;
	void setOrientation(float orientation);

	network::EntityType entityType() const;

	double current(attrib::Type type) const;

	double max(attrib::Type type) const;

	inline int visibleCount() const {
		return _visible.size();
	}

	/**
	 * @brief Allows to execute a functor/lambda on the visible objects
	 * @note This is thread safe
	 */
	template<typename Func>
	void visitVisible(Func&& func) {
		core::ScopedReadLock lock(_visibleLock);
		for (const EntityPtr& e : _visible) {
			func(e);
		}
	}

	/**
	 * @brief Creates a copy of the currently visible objects. If you don't need a copy, use the @c Entity::visibleVisible method.
	 * @note This is thread safe
	 */
	inline EntitySet visibleCopy() const {
		core::ScopedReadLock lock(_visibleLock);
		const EntitySet set(_visible);
		return set;
	}

	/**
	 * @brief This will inform the entity about all the other entities that it can see.
	 * @param[in] set The entities that are currently visible
	 * @note All entities have the same view range - see @c Entity::regionRect
	 * @note This is thread safe
	 */
	void updateVisible(const EntitySet& set);

	/**
	 * @brief The tick of the entity
	 * @param[in] dt The delta time (in millis) since the last tick was executed
	 * @return @c false if the entity should be removed from the world
	 */
	virtual bool update(long dt);

	virtual void init();

	/**
	 * @return the size of this entity that is used for the visibility checks
	 */
	float size() const;

	/**
	 * @brief Calculates the two dimensional rect that defines the size of the entity.
	 * @note The position is in the center of this rectangle.
	 * @note This is in world coordinates.
	 */
	core::RectFloat rect() const;

	/**
	 * @brief the view rect defines which rect the entity can see right now.
	 * This is used e.g. for visibility calculation
	 */
	core::RectFloat viewRect() const;

	/**
	 * @brief Check whether the given position can be seen by the entity.
	 * @param[in] position The position to check
	 * @return @c true if the position is in the current frustum of the entity.
	 */
	bool inFrustum(const glm::vec3& position) const;
	bool inFrustum(const Entity& other) const;
	bool inFrustum(const EntityPtr& other) const;
};

inline MapPtr Entity::map() const {
	return _map;
}

inline double Entity::current(attrib::Type type) const {
	return _attribs.current(type);
}

inline double Entity::max(attrib::Type type) const {
	return _attribs.max(type);
}

inline network::EntityType Entity::entityType() const {
	return _entityType;
}

inline glm::vec3 Entity::pos() const {
	return _pos;
}

inline void Entity::setPos(const glm::vec3& pos) {
	_pos = pos;
}

inline float Entity::size() const {
	return _size;
}

inline float Entity::orientation() const {
	return _orientation;
}

inline void Entity::setOrientation(float orientation) {
	_orientation = orientation;
}

inline cooldown::CooldownMgr& Entity::cooldownMgr() {
	return _cooldowns;
}

inline EntityId Entity::id() const {
	return _entityId;
}

inline void Entity::setMap(const MapPtr& map, const glm::vec3& pos) {
	_map = map;
	_pos = pos;
}

inline bool Entity::dead() const {
	return _attribs.current(attrib::Type::HEALTH) < 0.00001;
}

inline ENetPeer* Entity::peer() const {
	if (_peer != nullptr && _peer->state == ENET_PEER_STATE_DISCONNECTED) {
		return nullptr;
	}
	return _peer;
}

inline bool Entity::inFrustum(const Entity& other) const {
	return inFrustum(other.pos());
}

inline bool Entity::inFrustum(const EntityPtr& other) const {
	return inFrustum(other->pos());
}

}
