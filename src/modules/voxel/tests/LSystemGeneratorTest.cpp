#include "AbstractVoxelTest.h"
#include "voxel/generator/LSystemGenerator.h"
#include "voxel/polyvox/Region.h"
#include "voxel/polyvox/PagedVolume.h"

namespace voxel {

class LSystemGeneratorTest: public AbstractVoxelTest {
protected:
	Pager _pager;
	TerrainContext _ctx;
	core::Random _random;
	long seed = 0;

public:
	void SetUp() override {
		AbstractVoxelTest::SetUp();
		_random.setSeed(seed);
		const voxel::Region region(glm::ivec3(0, 0, 0), glm::ivec3(63, 63, 63));
		_ctx.region = region;
		_ctx.voxelStorage = nullptr;
	}
};

TEST_F(LSystemGeneratorTest, testStatePushPop) {
	LSystemContext lsystemCtx;
	// we change the coordinates in x, y and z directions once, then we push a new state and pop that
	// new state again - which means, that we don't modify the initial state => hence the 1, 1, 1
	lsystemCtx.axiom = "XYZ[XYZ]";

	LSystemState state;
	LSystemGenerator::expand(&state, _ctx, lsystemCtx, _random, lsystemCtx.axiom, lsystemCtx.generations);

	ASSERT_EQ(1, state.pos.x);
	ASSERT_EQ(1, state.pos.y);
	ASSERT_EQ(1, state.pos.z);
}

TEST_F(LSystemGeneratorTest, testStatePushPopPositionChangeToInit) {
	LSystemContext lsystemCtx;
	lsystemCtx.axiom = "XYZ[XYZ]xyz";

	LSystemState state;
	LSystemGenerator::expand(&state, _ctx, lsystemCtx, _random, lsystemCtx.axiom, lsystemCtx.generations);

	ASSERT_EQ(0, state.pos.x);
	ASSERT_EQ(0, state.pos.y);
	ASSERT_EQ(0, state.pos.z);
}

TEST_F(LSystemGeneratorTest, testMultipleStates) {
	LSystemContext lsystemCtx;
	lsystemCtx.axiom = "AY[xYA]AY[XYA]AY";
	lsystemCtx.productionRules.emplace('A', lsystemCtx.axiom);
	lsystemCtx.voxels.emplace('A', createVoxel(Wood));
	lsystemCtx.generations = 2;

	LSystemState state;
	LSystemGenerator::expand(&state, _ctx, lsystemCtx, _random, lsystemCtx.axiom, lsystemCtx.generations);

	ASSERT_EQ(0, state.pos.x);
	ASSERT_EQ(12, state.pos.y);
	ASSERT_EQ(0, state.pos.z);
}

TEST_F(LSystemGeneratorTest, testStatePositionChangeTwice) {
	LSystemContext lsystemCtx;
	lsystemCtx.axiom = "XYZXYZ";

	LSystemState state;
	LSystemGenerator::expand(&state, _ctx, lsystemCtx, _random, lsystemCtx.axiom, lsystemCtx.generations);

	ASSERT_EQ(2, state.pos.x);
	ASSERT_EQ(2, state.pos.y);
	ASSERT_EQ(2, state.pos.z);
}

TEST_F(LSystemGeneratorTest, testGenerateVoxels) {
	LSystemContext lsystemCtx;
	lsystemCtx.axiom = "AB";
	lsystemCtx.generations = 2;

	lsystemCtx.productionRules.emplace('A', "XAxYAXBXXYYZZ");
	lsystemCtx.productionRules.emplace('B', "A[zC]");

	lsystemCtx.voxels.emplace('A', createVoxel(Wood));
	lsystemCtx.voxels.emplace('B', createVoxel(Grass));
	lsystemCtx.voxels.emplace('C', createVoxel(Leaves4));

	LSystemGenerator::generate(_ctx, lsystemCtx, _random);
}

}
