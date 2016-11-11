/**
 * @file
 */

#pragma once

#include "ShapeGenerator.h"

namespace voxel {

class GeneratorContext;

extern void createPlanet(GeneratorContext& ctx, const glm::vec3& center, const Voxel& voxel, float scale = 1.0f);

}
