#pragma once

#include "core/EventBus.h"
#include "backend/ForwardDecl.h"
#include "Shared_generated.h"

namespace backend {

#define ENTITYEVENT(name) EVENTBUSPAYLOADEVENT(name, EntityPtr);

#define ENTITYIDTYPEEVENT(name) \
	class name: public core::IEventBusEvent { \
	private: \
		EntityId _entityId; \
		network::EntityType _type; \
		name() : _entityId(EntityIdNone), _type(network::EntityType::NONE) {} \
	public: \
		EVENTBUSTYPEID(name) \
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
