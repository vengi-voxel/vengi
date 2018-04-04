/**
 * @file
 */
#pragma once

#include "core/Common.h"
#include <vector>
#include <glm/vec3.hpp>

namespace voxel {
class WorldMgr;
typedef std::shared_ptr<WorldMgr> WorldMgrPtr;
}

namespace frontend {

extern void distributePlants(const voxel::WorldMgrPtr& world, const glm::ivec3& pos, std::vector<glm::vec3>& translations);

}
