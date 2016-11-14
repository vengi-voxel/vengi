/**
 * @file
 */

#pragma once

#include "core/Random.h"
#include "voxel/BiomeManager.h"
#include "voxel/WorldContext.h"
#include "voxel/Spiral.h"
#include "ShapeGenerator.h"
#include "LSystemGenerator.h"

namespace voxel {
namespace tree {

template<class Volume>
static int findFloor(const Volume& volume, int x, int z) {
	for (int i = MAX_TERRAIN_HEIGHT - 1; i >= MAX_WATER_HEIGHT; i--) {
		const VoxelType material = volume.getVoxel(x, i, z).getMaterial();
		if (isLeaves(material)) {
			return -1;
		}
		if (!isRock(material) && (isFloor(material) || isWood(material))) {
			return i + 1;
		}
	}
	return -1;
}

static inline Voxel getLeavesVoxel(core::Random& random) {
	const VoxelType leavesType = (VoxelType)random.random(std::enum_value(VoxelType::Leaves1), std::enum_value(VoxelType::Leaves10));
	const Voxel leavesVoxel = createVoxel(leavesType);
	return leavesVoxel;
}

template<class Volume>
void createTreeBranchEllipsis(Volume& volume, const TreeContext& ctx, core::Random& random) {
	const int top = ctx.treeTop();
	static constexpr Voxel voxel = createVoxel(VoxelType::Wood1);
	shape::createCubeNoCenter(volume, ctx.pos - glm::ivec3(1), ctx.trunkWidth + 2, 1, ctx.trunkWidth + 2, voxel);
	shape::createCubeNoCenter(volume, ctx.pos, ctx.trunkWidth, ctx.trunkHeight, ctx.trunkWidth, voxel);
	if (ctx.trunkHeight <= 8) {
		return;
	}
	const Voxel leavesVoxel = getLeavesVoxel(random);
	std::vector<int> branches = {1, 2, 3, 4};
	random.shuffle(branches.begin(), branches.end());
	const int n = random.random(1, 4);
	for (int i = 0; i < n; ++i) {
		const int thickness = std::max(2, ctx.trunkWidth / 2);
		const int branchHeight = ctx.trunkHeight / 2;
		const int branchSize = random.random(thickness * 2, std::max(thickness * 2, ctx.trunkWidth));

		glm::ivec3 branch = ctx.pos;
		branch.y = random.random(ctx.pos.y + 2, top - 2);

		const int delta = (ctx.trunkWidth - thickness) / 2;
		glm::ivec3 leavesPos;
		switch (branches[i]) {
		case 1:
			branch.x += delta;
			leavesPos = shape::createL(volume, branch, 0, branchSize, branchHeight, thickness, voxel);
			break;
		case 2:
			branch.x += delta;
			leavesPos = shape::createL(volume, branch, 0, -branchSize, branchHeight, thickness, voxel);
			break;
		case 3:
			branch.z += delta;
			leavesPos = shape::createL(volume, branch, branchSize, 0, branchHeight, thickness, voxel);
			break;
		case 4:
			branch.z += delta;
			leavesPos = shape::createL(volume, branch, -branchSize, 0, branchHeight, thickness, voxel);
			break;
		}
		leavesPos.y += branchHeight / 2;
		shape::createEllipse(volume, leavesPos, branchHeight, branchHeight, branchHeight, leavesVoxel);
	}
	const glm::ivec3 leafesPos(ctx.pos.x + ctx.trunkWidth / 2, top + ctx.leavesHeight / 2, ctx.pos.z + ctx.trunkWidth / 2);
	shape::createEllipse(volume, leafesPos, ctx.leavesWidth, ctx.leavesHeight, ctx.leavesDepth, leavesVoxel);
}

template<class Volume>
static void createTrunk(Volume& volume, const TreeContext& ctx) {
	const int top = ctx.treeTop();
	static constexpr Voxel voxel = createVoxel(VoxelType::Wood1);
	for (int y = ctx.pos.y; y < top; ++y) {
		const int trunkWidthY = ctx.trunkWidth + std::max(0, 2 - (y - ctx.pos.y));
		for (int x = ctx.pos.x - trunkWidthY; x < ctx.pos.x + trunkWidthY; ++x) {
			for (int z = ctx.pos.z - trunkWidthY; z < ctx.pos.z + trunkWidthY; ++z) {
				if ((x >= ctx.pos.x + trunkWidthY || x < ctx.pos.x - trunkWidthY)
						&& (z >= ctx.pos.z + trunkWidthY || z < ctx.pos.z - trunkWidthY)) {
					continue;
				}
				glm::ivec3 finalPos(x, y, z);
				if (y == ctx.pos.y) {
					finalPos.y = findFloor(volume, x, z);
					if (finalPos.y < 0) {
						continue;
					}
					for (int i = finalPos.y + 1; i <= y; ++i) {
						volume.setVoxel(finalPos.x, i, finalPos.z, voxel);
					}
				}

				volume.setVoxel(finalPos, voxel);
			}
		}
	}
}

template<class Volume>
void createTreeEllipsis(Volume& volume, const TreeContext& ctx, core::Random& random) {
	const int top = ctx.pos.y + ctx.trunkHeight;
	const Voxel leavesVoxel = getLeavesVoxel(random);

	createTrunk(volume, ctx);

	const glm::ivec3 leafesPos(ctx.pos.x, top + ctx.leavesHeight / 2, ctx.pos.z);
	shape::createEllipse(volume, leafesPos, ctx.leavesWidth, ctx.leavesHeight, ctx.leavesDepth, leavesVoxel);
}

template<class Volume>
void createTreeCone(Volume& volume, const TreeContext& ctx, core::Random& random) {
	const int top = ctx.treeTop();
	const Voxel leavesVoxel = getLeavesVoxel(random);

	createTrunk(volume, ctx);

	const glm::ivec3 leafesPos(ctx.pos.x, top + ctx.leavesHeight / 2, ctx.pos.z);
	shape::createCone(volume, leafesPos, ctx.leavesWidth, ctx.leavesHeight, ctx.leavesDepth, leavesVoxel);
}

template<class Volume>
void createTreeFir(Volume& volume, const TreeContext& ctx, core::Random& random) {
	const int top = ctx.treeTop();
	const Voxel leavesVoxel = getLeavesVoxel(random);

	createTrunk(volume, ctx);

	const int branches = 12;
	const float stepWidth = glm::radians(360.0f / branches);
	float angle = random.random(0, glm::two_pi<float>());
	float w = 1.3f;
	const int amount = 3;
	const int stepHeight = 10;
	glm::ivec3 leafesPos(ctx.pos.x, top, ctx.pos.z);

	const int halfHeight = ((amount - 1) * stepHeight) / 2;
	glm::ivec3 center(ctx.pos.x, top - halfHeight, ctx.pos.z);
	shape::createCube(volume, center, ctx.trunkWidth * 3, halfHeight * 2, ctx.trunkWidth * 3, leavesVoxel);

	for (int n = 0; n < amount; ++n) {
		for (int b = 0; b < branches; ++b) {
			glm::ivec3 start = leafesPos;
			glm::ivec3 end = start;
			const float x = glm::cos(angle);
			const float z = glm::sin(angle);
			const int randomZ = random.random(4, 8);
			end.y -= randomZ;
			end.x -= x * w;
			end.z -= z * w;
			shape::createLine(volume, start, end, leavesVoxel);
			glm::ivec3 end2 = end;
			end2.y -= 4;
			end2.x -= x * w * 1.8;
			end2.z -= z * w * 1.8;
			shape::createLine(volume, end, end2, leavesVoxel);
			angle += stepWidth;
			w += 1.0 / (double)(b + 1);
		}
		leafesPos.y -= stepHeight;
	}
}

template<class Volume>
void createTreePine(Volume& volume, const TreeContext& ctx, core::Random& random) {
	createTrunk(volume, ctx);

	core_assert_msg(ctx.leavesHeight <= ctx.trunkHeight, "The trunk must be >= than the height of the leaves for this tree type");

	const int singleLeaveHeight = 2;
	const int singleStepDelta = 1;
	const int singleStepHeight = singleLeaveHeight + singleStepDelta;
	const int steps = std::max(1, ctx.leavesHeight / singleStepHeight);
	const int stepWidth = ctx.leavesWidth / steps;
	const int stepDepth = ctx.leavesDepth / steps;
	int currentWidth = 2;
	int currentDepth = 2;
	const int top = ctx.pos.y + ctx.trunkHeight;
	glm::ivec3 leavesPos(ctx.pos.x, top, ctx.pos.z);
	const Voxel leavesVoxel = getLeavesVoxel(random);
	for (int i = 0; i < steps; ++i) {
		shape::createDome(volume, leavesPos, currentWidth, singleLeaveHeight, currentDepth, leavesVoxel);
		leavesPos.y -= singleStepDelta;
		shape::createDome(volume, leavesPos, currentWidth + 1, singleLeaveHeight, currentDepth + 1, leavesVoxel);
		currentDepth += stepDepth;
		currentWidth += stepWidth;
		leavesPos.y -= singleLeaveHeight;
	}
}

template<class Volume>
void createTreeDome(Volume& volume, const TreeContext& ctx, core::Random& random) {
	const int top = ctx.treeTop();
	const Voxel leavesVoxel = getLeavesVoxel(random);

	createTrunk(volume, ctx);

	const glm::ivec3 leafesPos(ctx.pos.x, top + ctx.leavesHeight / 2, ctx.pos.z);
	if (random.randomf() < 0.5f) {
		shape::createDome(volume, leafesPos, ctx.leavesWidth, ctx.leavesHeight, ctx.leavesDepth, leavesVoxel);
	} else {
		const glm::ivec3 trunkPos(ctx.pos.x, top, ctx.pos.z);
		shape::createDome(volume, leafesPos, ctx.leavesWidth, ctx.leavesHeight, ctx.leavesDepth, leavesVoxel);
		int branches = 6;
		const float stepWidth = glm::radians(360.0f / branches);
		float angle = random.random(0, glm::two_pi<float>());
		for (int b = 0; b < branches; ++b) {
			glm::ivec3 start = trunkPos;
			const float x = glm::cos(angle);
			const float z = glm::sin(angle);
			start.x -= x * (ctx.leavesWidth - 1) / 2;
			start.z -= z * (ctx.leavesDepth - 1) / 2;
			const int randomZ = random.random(4, 8);
			glm::ivec3 end = start;
			end.y -= randomZ;
			shape::createLine(volume, start, end, leavesVoxel);
			angle += stepWidth;
		}
	}
}

template<class Volume>
void createTreeCube(Volume& volume, const TreeContext& ctx, core::Random& random) {
	const int top = ctx.treeTop();
	const Voxel leavesVoxel = getLeavesVoxel(random);

	createTrunk(volume, ctx);

	const glm::ivec3 leafesPos(ctx.pos.x, top + ctx.leavesHeight / 2, ctx.pos.z);
	shape::createCube(volume, leafesPos, ctx.leavesWidth, ctx.leavesHeight, ctx.leavesDepth, leavesVoxel);
	// TODO: use CreatePlane
	shape::createCube(volume, leafesPos, ctx.leavesWidth + 2, ctx.leavesHeight - 2, ctx.leavesDepth - 2, leavesVoxel);
	shape::createCube(volume, leafesPos, ctx.leavesWidth - 2, ctx.leavesHeight + 2, ctx.leavesDepth - 2, leavesVoxel);
	shape::createCube(volume, leafesPos, ctx.leavesWidth - 2, ctx.leavesHeight - 2, ctx.leavesDepth + 2, leavesVoxel);
	if (random.randomf() < 0.5f) {
		Spiral o;
		o.next();
		const int halfWidth = ctx.leavesWidth / 2;
		const int halfHeight = ctx.leavesHeight / 2;
		const int halfDepth = ctx.leavesDepth / 2;
		for (int i = 0; i < 4; ++i) {
			glm::ivec3 leafesPos2 = leafesPos;
			leafesPos2.x += o.x() * halfWidth;
			leafesPos2.z += o.z() * halfDepth;
			shape::createEllipse(volume, leafesPos2, halfWidth, halfHeight, halfDepth, leavesVoxel);
			o.next(2);
		}
	}
}

template<class Volume>
void createTree(Volume& volume, const TreeContext& ctx, core::Random& random) {
	if (ctx.type == TreeType::BranchesEllipsis) {
		createTreeBranchEllipsis(volume, ctx, random);
	} else if (ctx.type == TreeType::Ellipsis) {
		createTreeEllipsis(volume, ctx, random);
	} else if (ctx.type == TreeType::Cone) {
		createTreeCone(volume, ctx, random);
	} else if (ctx.type == TreeType::Fir) {
		createTreeFir(volume, ctx, random);
	} else if (ctx.type == TreeType::Pine) {
		createTreePine(volume, ctx, random);
	} else if (ctx.type == TreeType::Dome) {
		createTreeDome(volume, ctx, random);
	} else if (ctx.type == TreeType::Cube) {
		createTreeCube(volume, ctx, random);
	}
}

template<class Volume>
void createTrees(Volume& volume, const Region& region, const BiomeManager& biomManager, core::Random& random) {
	const int amount = biomManager.getAmountOfTrees(region);
	TreeContext ctx;
	for (int i = 0; i < amount; ++i) {
		const int regionBorder = 8;
		const int rndValX = random.random(regionBorder, region.getWidthInVoxels() - regionBorder);
		// number should be even - to get more variance
		if (!(rndValX % 2)) {
			continue;
		}

		const int rndValZ = random.random(regionBorder, region.getDepthInVoxels() - regionBorder);
		ctx.pos = glm::ivec3(region.getLowerX() + rndValX, -1, region.getLowerZ() + rndValZ);
		const int y = findFloor(volume, ctx.pos.x, ctx.pos.z);
		if (y < 0) {
			continue;
		}

		ctx.pos.y = y;

		if (!biomManager.hasTrees(ctx.pos)) {
			continue;
		}

		ctx.trunkWidth = 1;
		const int maxSize = 18;
		int size = random.random(12, maxSize);
		ctx.leavesWidth = size;
		ctx.leavesDepth = size;
		ctx.type = (TreeType)random.random(0, int(TreeType::Max) - 1);
		switch (ctx.type) {
		case TreeType::Fir:
			ctx.leavesHeight = random.random(20, 28);
			ctx.trunkHeight = random.random(40, 56);
			break;
		case TreeType::Pine:
			ctx.leavesHeight = random.random(20, 28);
			ctx.trunkHeight = random.random(20, 28);
			break;
		case TreeType::Cone:
			ctx.leavesHeight = random.random(20, 28);
			ctx.trunkHeight = random.random(5, 9);
			break;
		case TreeType::Dome:
			ctx.leavesHeight = random.random(20, 28);
			ctx.trunkHeight = random.random(10, 14);
			break;
		case TreeType::BranchesEllipsis:
			ctx.leavesHeight = random.random(10, 14);
			ctx.trunkHeight = random.random(6, 15);
			ctx.trunkWidth = 4;
			break;
		default:
			ctx.leavesHeight = random.random(10, 14);
			ctx.trunkHeight = random.random(5, 9);
			break;
		}
		createTree(volume, ctx, random);
	}
}

}
}
