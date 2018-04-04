/**
 * @file
 */

#pragma once

#include "core/EventBus.h"
#include "voxel/WorldMgr.h"

namespace voxel {

class WorldCreatedEvent: public core::IEventBusEvent {
private:
	WorldMgrPtr _world;
public:
	WorldCreatedEvent(const WorldMgrPtr& world) :
			_world(world) {
	}

	inline const WorldMgrPtr& world() const {
		return _world;
	}
};

}
