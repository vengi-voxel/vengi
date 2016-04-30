#include "TreeGenerator.h"
#include "LSystemGenerator.h"
#include "voxel/Voxel.h"

namespace voxel {

int TreeGenerator::findFloor(const PagedVolume* volume, int x, int z) {
	for (int i = MAX_TERRAIN_HEIGHT - 1; i >= 0; i--) {
		const int material = volume->getVoxel(x, i, z).getMaterial();
		if (isLeaves(material)) {
			return -1;
		}
		if (isFloor(material) || isWood(material)) {
			return i + 1;
		}
	}
	return -1;
}

void TreeGenerator::createTrees(TerrainContext& ctx, core::Random& random) {
	const Region& region = ctx.region;
	for (int i = 0; i < 5; ++i) {
		const int regionBorder = 1;
		const int rndValX = random.random(regionBorder, region.getWidthInVoxels() - regionBorder);
		// number should be even
		if (!(rndValX % 2)) {
			continue;
		}

		const int rndValZ = random.random(regionBorder, region.getDepthInVoxels() - regionBorder);
		// TODO: use a noise map to get the position
		glm::ivec3 pos(region.getLowerX() + rndValX, -1, region.getLowerZ() + rndValZ);
		const int y = findFloor(ctx.volume, pos.x, pos.z);
		const int height = random.random(10, 14);
		const int trunkHeight = random.random(5, 9);
		if (y < 0) {
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
	if (type == TreeType::LSYSTEM) {
		// TODO: select leave type via rule
		const VoxelType leavesType = random.random(Leaves1, Leaves10);
		const Voxel leavesVoxel = createVoxel(leavesType);
		LSystemContext lsystemCtx;
		// TODO: improve rule
		lsystemCtx.axiom = "AY[xYA]AY[XYA]AY";
		lsystemCtx.productionRules.emplace('A', lsystemCtx.axiom);
		lsystemCtx.voxels.emplace('A', leavesVoxel);
		lsystemCtx.generations = 2;
		lsystemCtx.start = pos;
		LSystemGenerator::generate(ctx, lsystemCtx, random);
		return;
	}

	int top = (int) pos.y + trunkHeight;
	if (type == TreeType::PINE) {
		top += height;
	}

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
					finalPos.y = findFloor(ctx.volume, x, z);
					if (finalPos.y < 0) {
						continue;
					}
					for (int i = finalPos.y + 1; i <= y; ++i) {
						ctx.volume->setVoxel(finalPos.x, i, finalPos.z, voxel);
					}
				}

				ctx.volume->setVoxel(finalPos.x, finalPos.y, finalPos.z, voxel);
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
