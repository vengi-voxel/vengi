/**
 * @file
 */
#pragma once

#include "core/Common.h"
#include <memory>
#include <vector>
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>

namespace voxel {
class WorldMgr;
typedef std::shared_ptr<WorldMgr> WorldMgrPtr;
}

namespace voxelrender {

extern void distributePlants(const voxel::WorldMgrPtr& world, const glm::ivec3& pos, std::vector<glm::vec3>& translations);

}
