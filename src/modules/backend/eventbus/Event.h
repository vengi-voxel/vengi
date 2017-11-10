#pragma once

#include "core/EventBus.h"
#include "backend/ForwardDecl.h"
#include "Shared_generated.h"

namespace backend {

#define ENTITYEVENT(name) \
	class name: public core::IEventBusEvent { \
	private: \
		EntityPtr _entity; \
	public: \
		name(const EntityPtr& entity) : _entity(entity) {} \
		inline const EntityPtr& entity() const { return _entity; } \
	};

#define ENTITYIDTYPEEVENT(name) \
	class name: public core::IEventBusEvent { \
	private: \
		EntityId _entityId; \
		network::EntityType _type; \
	public: \
		name(EntityId entityId, network::EntityType type) : _entityId(entityId), _type(type) {} \
		inline EntityId entityId() const { return _entityId; } \
		inline network::EntityType entityType() const { return _type; } \
	};

/**
 * @brief Remove an entity from the map/world. But it can be readded later
 *
 * @see @c EntityDeleteEvent
 */
ENTITYEVENT(EntityRemoveFromMapEvent)

ENTITYEVENT(EntityAddToMapEvent)

/**
 * @brief Delete an entity from the server
 *
 * @note This event doesn't hold a reference to the entity - it might already be invalid.
 *
 * @see @c EntityRemoveFromMapEvent
 */
ENTITYIDTYPEEVENT(EntityDeleteEvent)

ENTITYEVENT(EntityAddEvent)

}
