#pragma once

#include "WorldContext.h"
#include "Voxel.h"

namespace voxel {

class WorldGenerator {
private:
	bool isValidChunkPosition(const TerrainContext& ctx, const glm::ivec3& pos) const;
	void setVolumeVoxel(TerrainContext& ctx, const glm::ivec3& pos, const Voxel& voxel);
public:
	void createCirclePlane(TerrainContext& ctx, const glm::ivec3& center, int width, int depth, double radius, const Voxel& voxel);
	void createCube(TerrainContext& ctx, const glm::ivec3& center, int width, int height, int depth, const Voxel& voxel);
	void createPlane(TerrainContext& ctx, const glm::ivec3& center, int width, int depth, const Voxel& voxel);
	void createEllipse(TerrainContext& ctx, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel);
	void createCone(TerrainContext& ctx, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel);
	void createDome(TerrainContext& ctx, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel);

};

}
