/**
 * @file
 */

#pragma once

#include "core/EventBus.h"
#include "voxel/World.h"

namespace voxel {

class WorldCreatedEvent: public core::IEventBusEvent {
private:
	WorldPtr _world;
public:
	WorldCreatedEvent(const WorldPtr& world) :
			_world(world) {
	}

	inline const WorldPtr& world() const {
		return _world;
	}
};

}
