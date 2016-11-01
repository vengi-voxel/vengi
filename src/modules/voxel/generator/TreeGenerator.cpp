/**
 * @file
 */

#include "TreeGenerator.h"
#include "LSystemGenerator.h"
#include "voxel/WorldContext.h"
#include "voxel/polyvox/Voxel.h"
#include "voxel/Spiral.h"

namespace voxel {

int TreeGenerator::findFloor(const GeneratorContext& ctx, int x, int z) {
	for (int i = MAX_TERRAIN_HEIGHT - 1; i >= MAX_WATER_HEIGHT; i--) {
		const VoxelType material = ctx.getVoxel(x, i, z).getMaterial();
		if (isLeaves(material)) {
			return -1;
		}
		if (!isRock(material) && (isFloor(material) || isWood(material))) {
			return i + 1;
		}
	}
	return -1;
}

void TreeGenerator::createTrees(GeneratorContext& ctx, const BiomeManager& biomManager, core::Random& random) {
	const Region& region = ctx.region;
	const int amount = biomManager.getAmountOfTrees(region);
	for (int i = 0; i < amount; ++i) {
		const int regionBorder = 8;
		const int rndValX = random.random(regionBorder, region.getWidthInVoxels() - regionBorder);
		// number should be even - to get more variance
		if (!(rndValX % 2)) {
			continue;
		}

		const int rndValZ = random.random(regionBorder, region.getDepthInVoxels() - regionBorder);
		glm::ivec3 pos(region.getLowerX() + rndValX, -1, region.getLowerZ() + rndValZ);
		const int y = findFloor(ctx, pos.x, pos.z);
		if (y < 0) {
			continue;
		}

		pos.y = y;

		if (!biomManager.hasTrees(pos)) {
			continue;
		}

		int height;
		int trunkHeight;
		int trunkWidth = 1;
		const int maxSize = 18;
		int size = random.random(12, maxSize);
		const TreeType treeType = (TreeType)random.random(0, int(TreeType::MAX) - 1);
		switch (treeType) {
		default:
			height = random.random(10, 14);
			trunkHeight = random.random(5, 9);
			break;
		case TreeType::FIR:
			height = random.random(20, 28);
			trunkHeight = random.random(40, 56);
			break;
		case TreeType::PINE:
			height = random.random(20, 28);
			trunkHeight = random.random(20, 28);
			break;
		case TreeType::CONE:
			height = random.random(20, 28);
			trunkHeight = random.random(5, 9);
			break;
		case TreeType::DOME:
			height = random.random(20, 28);
			trunkHeight = random.random(10, 14);
			break;
		case TreeType::CACTUS:
			height = random.random(20, 28);
			trunkHeight = random.random(10, 14);
			trunkWidth = 4;
			break;
		case TreeType::BRANCHESELLIPSIS:
			height = random.random(10, 14);
			trunkHeight = random.random(6, 15);
			trunkWidth = 4;
			break;
		}

		addTree(ctx, pos, treeType, trunkHeight, trunkWidth, size, size, height, random);
	}
}

void TreeGenerator::addTree(GeneratorContext& ctx, const glm::ivec3& pos, TreeType type, int trunkHeight, int trunkWidth, int width, int depth, int height, core::Random& random) {
	int top = (int) pos.y + trunkHeight;
	static constexpr Voxel voxel = createVoxel(VoxelType::Wood1);
	const VoxelType leavesType = (VoxelType)random.random(std::enum_value(VoxelType::Leaves1), std::enum_value(VoxelType::Leaves10));
	const Voxel leavesVoxel = createVoxel(leavesType);

	if (type == TreeType::LSYSTEM) {
		// TODO: select leave type via rule
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
	} else if (type == TreeType::CACTUS) {
		ShapeGenerator::createCubeNoCenter(ctx, pos, trunkWidth, trunkHeight, trunkWidth, leavesVoxel);
		std::vector<int> branches = {1, 2, 3, 4};
		random.shuffle(branches.begin(), branches.end());
		const int n = random.random(2, 4);
		for (int i = 0; i < n; ++i) {
			const int thickness = std::max(2, trunkWidth / 2);
			const int branchHeight = trunkHeight / 2;
			const int branchSize = random.random(thickness * 2, std::max(thickness * 2, trunkWidth));

			glm::ivec3 branch = pos;
			branch.y = random.random(pos.y + 2, top - 2);

			const int delta = (trunkWidth - thickness) / 2;
			switch (branches[i]) {
			case 1:
				branch.x += delta;
				ShapeGenerator::createL(ctx, branch, 0, branchSize, branchHeight, thickness, leavesVoxel);
				break;
			case 2:
				branch.x += delta;
				ShapeGenerator::createL(ctx, branch, 0, -branchSize, branchHeight, thickness, leavesVoxel);
				break;
			case 3:
				branch.z += delta;
				ShapeGenerator::createL(ctx, branch, branchSize, 0, branchHeight, thickness, leavesVoxel);
				break;
			case 4:
				branch.z += delta;
				ShapeGenerator::createL(ctx, branch, -branchSize, 0, branchHeight, thickness, leavesVoxel);
				break;
			}
		}
		return;
	} else if (type == TreeType::BRANCHESELLIPSIS) {
		ShapeGenerator::createCubeNoCenter(ctx, pos - glm::ivec3(1), trunkWidth + 2, 1, trunkWidth + 2, voxel);
		ShapeGenerator::createCubeNoCenter(ctx, pos, trunkWidth, trunkHeight, trunkWidth, voxel);
		if (trunkHeight >= 8) {
			std::vector<int> branches = {1, 2, 3, 4};
			random.shuffle(branches.begin(), branches.end());
			const int n = random.random(1, 4);
			for (int i = 0; i < n; ++i) {
				const int thickness = std::max(2, trunkWidth / 2);
				const int branchHeight = trunkHeight / 2;
				const int branchSize = random.random(thickness * 2, std::max(thickness * 2, trunkWidth));

				glm::ivec3 branch = pos;
				branch.y = random.random(pos.y + 2, top - 2);

				const int delta = (trunkWidth - thickness) / 2;
				glm::ivec3 leavesPos;
				switch (branches[i]) {
				case 1:
					branch.x += delta;
					leavesPos = ShapeGenerator::createL(ctx, branch, 0, branchSize, branchHeight, thickness, voxel);
					break;
				case 2:
					branch.x += delta;
					leavesPos = ShapeGenerator::createL(ctx, branch, 0, -branchSize, branchHeight, thickness, voxel);
					break;
				case 3:
					branch.z += delta;
					leavesPos = ShapeGenerator::createL(ctx, branch, branchSize, 0, branchHeight, thickness, voxel);
					break;
				case 4:
					branch.z += delta;
					leavesPos = ShapeGenerator::createL(ctx, branch, -branchSize, 0, branchHeight, thickness, voxel);
					break;
				}
				leavesPos.y += branchHeight / 2;
				ShapeGenerator::createEllipse(ctx, leavesPos, branchHeight, branchHeight, branchHeight, leavesVoxel);
			}
			const glm::ivec3 leafesPos(pos.x + trunkWidth / 2, top + height / 2, pos.z + trunkWidth / 2);
			ShapeGenerator::createEllipse(ctx, leafesPos, width, height, depth, leavesVoxel);
		}
		return;
	}

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
					finalPos.y = findFloor(ctx, x, z);
					if (finalPos.y < 0) {
						continue;
					}
					for (int i = finalPos.y + 1; i <= y; ++i) {
						ctx.setVoxel(finalPos.x, i, finalPos.z, voxel);
					}
				}

				ctx.setVoxel(finalPos, voxel);
			}
		}
	}

	if (type == TreeType::ELLIPSIS) {
		const glm::ivec3 leafesPos(pos.x, top + height / 2, pos.z);
		ShapeGenerator::createEllipse(ctx, leafesPos, width, height, depth, leavesVoxel);
	} else if (type == TreeType::CONE) {
		const glm::ivec3 leafesPos(pos.x, top + height / 2, pos.z);
		ShapeGenerator::createCone(ctx, leafesPos, width, height, depth, leavesVoxel);
	} else if (type == TreeType::FIR) {
		const int branches = 12;
		const float stepWidth = glm::radians(360.0f / branches);
		float angle = random.random(0, glm::two_pi<float>());
		float w = 1.3f;
		const int amount = 3;
		const int stepHeight = 10;
		glm::ivec3 leafesPos(pos.x, top, pos.z);

		const int halfHeight = ((amount - 1) * stepHeight) / 2;
		glm::ivec3 center(pos.x, top - halfHeight, pos.z);
		ShapeGenerator::createCube(ctx, center, trunkWidth * 3, halfHeight * 2, trunkWidth * 3, leavesVoxel);

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
				ShapeGenerator::createLine(ctx, start, end, leavesVoxel);
				glm::ivec3 end2 = end;
				end2.y -= 4;
				end2.x -= x * w * 1.8;
				end2.z -= z * w * 1.8;
				ShapeGenerator::createLine(ctx, end, end2, leavesVoxel);
				angle += stepWidth;
				w += 1.0 / (double)(b + 1);
			}
			leafesPos.y -= stepHeight;
		}
	} else if (type == TreeType::PINE) {
		const int singleLeaveHeight = 2;
		const int singleStepDelta = 1;
		const int singleStepHeight = singleLeaveHeight + singleStepDelta;
		const int steps = std::max(1, height / singleStepHeight);
		const int stepWidth = width / steps;
		const int stepDepth = depth / steps;
		int currentWidth = 2;
		int currentDepth = 2;
		glm::ivec3 leavesPos(pos.x, top, pos.z);
		for (int i = 0; i < steps; ++i) {
			ShapeGenerator::createDome(ctx, leavesPos, currentWidth, singleLeaveHeight, currentDepth, leavesVoxel);
			leavesPos.y -= singleStepDelta;
			ShapeGenerator::createDome(ctx, leavesPos, currentWidth + 1, singleLeaveHeight, currentDepth + 1, leavesVoxel);
			currentDepth += stepDepth;
			currentWidth += stepWidth;
			leavesPos.y -= singleLeaveHeight;
		}
	} else if (type == TreeType::DOME) {
		const glm::ivec3 leafesPos(pos.x, top + height / 2, pos.z);
		if (random.randomf() < 0.5f) {
			ShapeGenerator::createDome(ctx, leafesPos, width, height, depth, leavesVoxel);
		} else {
			const glm::ivec3 trunkPos(pos.x, top, pos.z);
			ShapeGenerator::createDome(ctx, leafesPos, width, height, depth, leavesVoxel);
			int branches = 6;
			const float stepWidth = glm::radians(360.0f / branches);
			float angle = random.random(0, glm::two_pi<float>());
			for (int b = 0; b < branches; ++b) {
				glm::ivec3 start = trunkPos;
				const float x = glm::cos(angle);
				const float z = glm::sin(angle);
				start.x -= x * (width - 1) / 2;
				start.z -= z * (depth - 1) / 2;
				const int randomZ = random.random(4, 8);
				glm::ivec3 end = start;
				end.y -= randomZ;
				ShapeGenerator::createLine(ctx, start, end, leavesVoxel);
				angle += stepWidth;
			}
		}
	} else if (type == TreeType::CUBE) {
		const glm::ivec3 leafesPos(pos.x, top + height / 2, pos.z);
		ShapeGenerator::createCube(ctx, leafesPos, width, height, depth, leavesVoxel);
		// TODO: use CreatePlane
		ShapeGenerator::createCube(ctx, leafesPos, width + 2, height - 2, depth - 2, leavesVoxel);
		ShapeGenerator::createCube(ctx, leafesPos, width - 2, height + 2, depth - 2, leavesVoxel);
		ShapeGenerator::createCube(ctx, leafesPos, width - 2, height - 2, depth + 2, leavesVoxel);
		if (random.randomf() < 0.5f) {
			Spiral o;
			o.next();
			const int halfWidth = width / 2;
			const int halfHeight = height / 2;
			const int halfDepth = depth / 2;
			for (int i = 0; i < 4; ++i) {
				glm::ivec3 leafesPos2 = leafesPos;
				leafesPos2.x += o.x() * halfWidth;
				leafesPos2.z += o.z() * halfDepth;
				ShapeGenerator::createEllipse(ctx, leafesPos2, halfWidth, halfHeight, halfDepth, leavesVoxel);
				o.next(2);
			}
		}
	}
}

}
