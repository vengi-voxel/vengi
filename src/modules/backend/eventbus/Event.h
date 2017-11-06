#pragma once

#include "core/EventBus.h"
#include "backend/ForwardDecl.h"

namespace backend {

#define ENTITYEVENT(name) \
	class name: public core::IEventBusEvent { \
	private: \
		EntityPtr _entity; \
	public: \
		name(const EntityPtr& entity) : _entity(entity) {} \
		inline const EntityPtr& entity() const { return _entity;  } \
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
 * @see @c EntityRemoveFromMapEvent
 */
ENTITYEVENT(EntityDeleteEvent)

ENTITYEVENT(EntityAddEvent)

}
