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
namespace cactus {

template<class Volume>
void createCactus(Volume& volume, const glm::ivec3& pos, TreeType type, int trunkHeight, int trunkWidth, int width, int depth, int height, core::Random& random) {
	std::vector<int> branches = {1, 2, 3, 4};
	random.shuffle(branches.begin(), branches.end());
	int top = (int) pos.y + trunkHeight;
	static constexpr Voxel voxel = createRandomColorVoxel(VoxelType::Wood);
	const Voxel leavesVoxel = createRandomColorVoxel(VoxelType::Leaves);
	const int n = random.random(2, 4);
	shape::createCubeNoCenter(volume, pos, trunkWidth, trunkHeight, trunkWidth, leavesVoxel);
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
			shape::createL(volume, branch, 0, branchSize, branchHeight, thickness, leavesVoxel);
			break;
		case 2:
			branch.x += delta;
			shape::createL(volume, branch, 0, -branchSize, branchHeight, thickness, leavesVoxel);
			break;
		case 3:
			branch.z += delta;
			shape::createL(volume, branch, branchSize, 0, branchHeight, thickness, leavesVoxel);
			break;
		case 4:
			branch.z += delta;
			shape::createL(volume, branch, -branchSize, 0, branchHeight, thickness, leavesVoxel);
			break;
		}
	}
}

}
}
