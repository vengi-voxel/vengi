/**
 * @file
 */

#pragma once

#include "ShapeGenerator.h"

namespace voxel {

class PagedVolumeWrapper;

extern void createPlanet(PagedVolumeWrapper& ctx, const glm::vec3& center, const Voxel& voxel, float scale = 1.0f);

}
