/**
 * @file
 */

#pragma once

#include "voxel/polyvox/Voxel.h"
#include "core/Common.h"
#include "core/Random.h"

namespace voxel {

struct WorldContext;
class GeneratorContext;
enum class TreeType;

class ShapeGenerator {
public:
	static void createCirclePlane(GeneratorContext& ctx, const glm::ivec3& center, int width, int depth, double radius, const Voxel& voxel);
	static void createCube(GeneratorContext& ctx, const glm::ivec3& center, int width, int height, int depth, const Voxel& voxel);
	static void createCubeNoCenter(GeneratorContext& ctx, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel);
	static void createPlane(GeneratorContext& ctx, const glm::ivec3& center, int width, int depth, const Voxel& voxel);
	static glm::ivec3 createL(GeneratorContext& ctx, const glm::ivec3& pos, int width, int depth, int height, int thickness, const Voxel& voxel);
	static void createEllipse(GeneratorContext& ctx, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel);
	static void createCone(GeneratorContext& ctx, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel);
	static void createDome(GeneratorContext& ctx, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel);
	static void createLine(GeneratorContext& ctx, const glm::ivec3& start, const glm::ivec3& end, const Voxel& voxel);
};

}
