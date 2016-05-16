/**
 * @file
 */

#pragma once

#include "voxel/Voxel.h"
#include "core/Common.h"
#include "core/Random.h"

namespace voxel {

struct WorldContext;
class TerrainContext;
enum class TreeType;

class ShapeGenerator {
public:
	static void createCirclePlane(TerrainContext& ctx, const glm::ivec3& center, int width, int depth, double radius, const Voxel& voxel);
	static void createCube(TerrainContext& ctx, const glm::ivec3& center, int width, int height, int depth, const Voxel& voxel);
	static void createPlane(TerrainContext& ctx, const glm::ivec3& center, int width, int depth, const Voxel& voxel);
	static void createEllipse(TerrainContext& ctx, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel);
	static void createCone(TerrainContext& ctx, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel);
	static void createDome(TerrainContext& ctx, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel);
	static void createLine(TerrainContext& ctx, const glm::ivec3& start, const glm::ivec3& end, const Voxel& voxel, int radius = 1);

};

}
