#pragma once

#include "core/EventBus.h"

namespace voxel {

class World;

class WorldCreatedEvent: public core::IEventBusEvent {
private:
	World* _world;
public:
	WorldCreatedEvent(World* world) :
			_world(world) {
	}

	inline World* world() const {
		return _world;
	}
};

}
