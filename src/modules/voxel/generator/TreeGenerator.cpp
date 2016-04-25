#include "TreeGenerator.h"

namespace voxel {

int TreeGenerator::findChunkFloor(int chunkHeight, PagedVolume::Chunk* chunk, int x, int z) {
	for (int i = chunkHeight - 1; i >= 0; i--) {
		const int material = chunk->getVoxel(x, i, z).getMaterial();
		if (isFloor(material)) {
			return i + 1;
		}
	}
	return -1;
}

void TreeGenerator::createTrees(TerrainContext& ctx, core::Random& random) {
	const Region& region = ctx.region;
	const int chunkHeight = region.getHeightInVoxels();
	for (int i = 0; i < 5; ++i) {
		const int regionBorder = 10;
		const int rndValX = random.random(regionBorder, region.getWidthInVoxels() - regionBorder);
		// number should be even
		if (!(rndValX % 2)) {
			continue;
		}

		const int rndValZ = random.random(regionBorder, region.getDepthInVoxels() - regionBorder);
		// TODO: use a noise map to get the position
		glm::ivec3 pos(rndValX, -1, rndValZ);
		const int y = findChunkFloor(chunkHeight, ctx.chunk, pos.x, pos.z);
		const int height = random.random(10, 14);
		const int trunkHeight = random.random(5, 9);
		if (y < 0 || y >= MAX_HEIGHT -1  - height - trunkHeight) {
			continue;
		}

		pos.y = y;

		const int maxSize = 14;
		const int size = random.random(12, maxSize);
		const int trunkWidth = 1;
		const TreeType treeType = (TreeType)random.random(0, int(TreeType::MAX) - 1);
		addTree(ctx, pos, treeType, trunkHeight, trunkWidth, size, size, height, random);
	}
}

void TreeGenerator::addTree(TerrainContext& ctx, const glm::ivec3& pos, TreeType type, int trunkHeight, int trunkWidth, int width, int depth, int height, core::Random& random) {
	int top = (int) pos.y + trunkHeight;
	if (type == TreeType::PINE) {
		top += height;
	}

	const int chunkHeight = ctx.region.getHeightInVoxels();

	const Voxel voxel = createVoxel(Wood);
	for (int y = pos.y; y < top; ++y) {
		const int trunkWidthY = trunkWidth + std::max(0, 2 - (y - pos.y));
		for (int x = pos.x - trunkWidthY; x < pos.x + trunkWidthY; ++x) {
			for (int z = pos.z - trunkWidthY; z < pos.z + trunkWidthY; ++z) {
				if ((x >= pos.x + trunkWidthY || x < pos.x - trunkWidthY)
						&& (z >= pos.z + trunkWidthY || z < pos.z - trunkWidthY)) {
					continue;
				}
				glm::ivec3 finalPos(x, y, z);
				if (y == pos.y) {
					core_assert(ShapeGenerator::isValidChunkPosition(ctx, finalPos));
					finalPos.y = findChunkFloor(chunkHeight, ctx.chunk, x, z);
					if (finalPos.y < 0) {
						continue;
					}
				}
				if (ShapeGenerator::isValidChunkPosition(ctx, finalPos)) {
					ctx.chunk->setVoxel(finalPos.x, finalPos.y, finalPos.z, voxel);
				} else {
					ShapeGenerator::setVolumeVoxel(ctx, finalPos, voxel);
				}
			}
		}
	}

	const VoxelType leavesType = random.random(Leaves1, Leaves10);
	const Voxel leavesVoxel = createVoxel(leavesType);
	const glm::ivec3 leafesPos(pos.x, top + height / 2, pos.z);
	if (type == TreeType::ELLIPSIS) {
		ShapeGenerator::createEllipse(ctx, leafesPos, width, height, depth, leavesVoxel);
	} else if (type == TreeType::CONE) {
		ShapeGenerator::createCone(ctx, leafesPos, width, height, depth, leavesVoxel);
	} else if (type == TreeType::PINE) {
		const int steps = std::max(1, height / 4);
		const int singleHeight = steps;
		const int stepWidth = width / steps;
		const int stepDepth = depth / steps;
		int currentWidth = stepWidth;
		int currentDepth = stepDepth;
		for (int i = 0; i < steps; ++i) {
			glm::ivec3 pineLeaves(pos.x, top - i * singleHeight, pos.z);
			ShapeGenerator::createDome(ctx, pineLeaves, currentWidth, singleHeight, currentDepth, leavesVoxel);
			pineLeaves.y -= 1;
			ShapeGenerator::createDome(ctx, pineLeaves, currentWidth + 1, singleHeight, currentDepth + 1, leavesVoxel);
			currentDepth += stepDepth;
			currentWidth += stepWidth;
		}
	} else if (type == TreeType::DOME) {
		ShapeGenerator::createDome(ctx, leafesPos, width, height, depth, leavesVoxel);
	} else if (type == TreeType::CUBE) {
		ShapeGenerator::createCube(ctx, leafesPos, width, height, depth, leavesVoxel);
		// TODO: use CreatePlane
		ShapeGenerator::createCube(ctx, leafesPos, width + 2, height - 2, depth - 2, leavesVoxel);
		ShapeGenerator::createCube(ctx, leafesPos, width - 2, height + 2, depth - 2, leavesVoxel);
		ShapeGenerator::createCube(ctx, leafesPos, width - 2, height - 2, depth + 2, leavesVoxel);
	}
}

}
