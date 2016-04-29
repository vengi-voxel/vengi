#include "AbstractVoxelTest.h"
#include "voxel/generator/LSystemGenerator.h"
#include "voxel/polyvox/Region.h"
#include "voxel/polyvox/PagedVolume.h"

namespace voxel {

class LSystemGeneratorTest: public AbstractVoxelTest {
};

TEST_F(LSystemGeneratorTest, testState) {
	const voxel::Region region(glm::ivec3(0, 0, 0), glm::ivec3(63, 63, 63));
	const long seed = 0;
	Pager pager;
	PagedVolume volData(&pager, 256 * 1024 * 1024, 64);
	TerrainContext ctx;
	ctx.region = region;
	ctx.volume = &volData;
	core::Random random;
	random.setSeed(seed);

	LSystemContext lsystemCtx;
	lsystemCtx.axiom = "XYZ[XYZ]";
	lsystemCtx.generations = 1;

	LSystemState state;
	state.pos = lsystemCtx.start;
	LSystemGenerator::expand(&state, ctx, lsystemCtx, random, lsystemCtx.axiom, lsystemCtx.generations);
	ASSERT_EQ(1, state.pos.x);
	ASSERT_EQ(1, state.pos.y);
	ASSERT_EQ(1, state.pos.z);
}

TEST_F(LSystemGeneratorTest, testGenerateVoxels) {
	const voxel::Region region(glm::ivec3(0, 0, 0), glm::ivec3(63, 63, 63));
	const long seed = 0;
	Pager pager;
	PagedVolume volData(&pager, 256 * 1024 * 1024, 64);
	TerrainContext ctx;
	ctx.region = region;
	ctx.volume = &volData;
	core::Random random;
	random.setSeed(seed);

	LSystemContext lsystemCtx;
	lsystemCtx.axiom = "AB";
	lsystemCtx.generations = 2;
	lsystemCtx.productionRules.emplace('A', "XAxYAXBXXYYZZ");
	lsystemCtx.productionRules.emplace('B', "A[zC]");
	lsystemCtx.voxels.emplace('A', createVoxel(Wood));
	lsystemCtx.voxels.emplace('B', createVoxel(Grass));
	lsystemCtx.voxels.emplace('C', createVoxel(Leaves4));
	lsystemCtx.start = glm::ivec3(0);

	LSystemGenerator::generate(ctx, lsystemCtx, random);
}

}
