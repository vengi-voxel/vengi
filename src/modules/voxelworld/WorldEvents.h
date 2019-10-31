/**
 * @file
 */

#pragma once

#include "core/EventBus.h"
#include "voxelworld/WorldMgr.h"

namespace voxelworld {

EVENTBUSPAYLOADEVENT(WorldCreatedEvent, WorldMgrPtr);
}
