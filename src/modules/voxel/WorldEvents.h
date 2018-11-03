/**
 * @file
 */

#pragma once

#include "core/EventBus.h"
#include "voxel/WorldMgr.h"

namespace voxel {

EVENTBUSPAYLOADEVENT(WorldCreatedEvent, WorldMgrPtr);
}
