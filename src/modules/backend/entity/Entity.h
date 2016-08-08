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
#include "network/MessageSender.h"
#include "EntityId.h"

namespace voxel {
class World;
typedef std::shared_ptr<World> WorldPtr;
}

namespace backend {

class Entity;
typedef std::shared_ptr<Entity> EntityPtr;
typedef std::unordered_set<EntityPtr> EntitySet;

class Entity {
private:
	core::ReadWriteLock _visibleLock;
	EntitySet _visible;

protected:
	EntityId _entityId;
	network::MessageSenderPtr _messageSender;
	attrib::ContainerProviderPtr _containerProvider;
	attrib::Attributes _attribs;
	cooldown::CooldownMgr _cooldowns;
	network::EntityType _npcType = network::EntityType::NONE;
	ENetPeer *_peer = nullptr;

	/**
	 * @brief Called with the set of entities that just get visible for this entity
	 */
	virtual void visibleAdd(const EntitySet& entities);
	/**
	 * @brief Called with the set of entities that just get invisible for this entity
	 */
	virtual void visibleRemove(const EntitySet& entities);
	void initAttribs();
public:

	Entity(EntityId id, const network::MessageSenderPtr& messageSender, const core::TimeProviderPtr& timeProvider, const attrib::ContainerProviderPtr& containerProvider) :
			_visibleLock("Entity"), _entityId(id), _messageSender(messageSender), _containerProvider(containerProvider), _cooldowns(timeProvider) {
	}

	void addContainer(const std::string& id);
	void removeContainer(const std::string& id);

	virtual ~Entity() {
	}

	inline cooldown::CooldownMgr& cooldownMgr() {
		return _cooldowns;
	}

	inline EntityId id() const {
		return _entityId;
	}

	inline bool dead() const {
		return _attribs.getCurrent(attrib::Types::HEALTH) < 0.00001;
	}

	inline ENetPeer* peer() const {
		if (_peer != nullptr && _peer->state == ENET_PEER_STATE_DISCONNECTED)
			return nullptr;
		return _peer;
	}

	/**
	 * @note The implementation behind this must ensure thread safety
	 * @return the current position in world coordinates
	 */
	virtual const glm::vec3& pos() const = 0;
	virtual float orientation() const = 0;
	network::EntityType npcType() const;

	inline double current(attrib::Types type) const {
		return _attribs.getCurrent(type);
	}

	inline double max(attrib::Types type) const {
		return _attribs.getMax(type);
	}

	/**
	 * @brief Allows to execute a functor/lambda on the visible objects
	 *
	 * @note This is thread safe
	 */
	template<typename Func>
	void visitVisible(Func& func) {
		core::ScopedReadLock lock(_visibleLock);
		for (const EntityPtr& e : _visible) {
			func(e);
		}
	}

	/**
	 * @brief Allows to execute a functor/lambda on the visible objects
	 *
	 * @note This is thread safe
	 */
	template<typename Func>
	void visitVisible(const Func& func) const {
		const EntitySet& copy = visibleCopy();
		for (const EntityPtr& e : copy) {
			func(e);
		}
	}

	/**
	 * @brief Creates a copy of the currently visible objects. If you don't need a copy, use the @c Entity::visibleVisible method.
	 *
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

	/**
	 * @return the size of this entity
	 */
	inline float size() const {
		return 1.0f;
	}

	/**
	 * @brief Calculates the two dimensional rect that defines the size of the entity.
	 * @note The position is in the center of this rectangle.
	 * @note This is in world coordinates.
	 */
	inline core::RectFloat rect() const {
		const glm::vec3 p = pos();
		const float halfSize = size() / 2.0f;
		return core::RectFloat(p.x - halfSize, p.z - halfSize, p.x + halfSize, p.z + halfSize);
	}

	/**
	 * @brief the region rect is where the player currently is - surrounded by 9 other fields.
	 * this is used e.g. for visibility calculation
	 */
	core::RectFloat regionRect() const {
		const glm::vec3 p = pos();
		static const float regionSize = 1000.0f;
		static const float regionHalf = regionSize / 2.0;
		return core::RectFloat(p.x - regionHalf, p.z - regionHalf, p.x + regionHalf, p.z + regionHalf);
	}
};

inline network::EntityType Entity::npcType() const {
	return _npcType;
}

}
