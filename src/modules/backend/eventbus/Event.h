#pragma once

#include "core/EventBus.h"
#include "backend/ForwardDecl.h"

namespace backend {

class EntityRemoveEvent: public core::IEventBusEvent {
private:
	EntityPtr _entity;
public:
	EntityRemoveEvent(const EntityPtr& entity) : _entity(entity) {
	}

	inline const EntityPtr& entity() const {
		return _entity;
	}
};

}
