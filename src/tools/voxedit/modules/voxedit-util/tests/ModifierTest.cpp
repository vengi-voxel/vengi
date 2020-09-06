/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "voxel/tests/TestHelper.h"
#include "../modifier/Modifier.h"
#include "core/ArrayLength.h"
#include "voxel/Voxel.h"

namespace voxedit {

class ModifierTest: public app::AbstractTest {
protected:
	void prepare(Modifier& modifier, const glm::ivec3 &mins, const glm::ivec3 &maxs, ModifierType modifierType) {
		modifier.setModifierType(modifierType);
		modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
		modifier.setGridResolution(1);
		modifier.setCursorPosition(mins, voxel::FaceNames::PositiveX);
		EXPECT_TRUE(modifier.aabbStart());
		EXPECT_TRUE(modifier.aabbMode());
		modifier.setCursorPosition(maxs, voxel::FaceNames::PositiveX);
		modifier.aabbStep();
	}

	void testMirror(math::Axis axis, const glm::ivec3& expectedMins, const glm::ivec3& expectedMaxs) {
		Modifier modifier;
		ASSERT_TRUE(modifier.init());
		const glm::ivec3 regMins(-2);
		const glm::ivec3 regMaxs = regMins;
		prepare(modifier, regMins, regMaxs, ModifierType::Place);
		EXPECT_TRUE(modifier.setMirrorAxis(axis, glm::ivec3(0)));
		const voxel::Region region(-3, 3);
		voxel::RawVolume volume(region);
		int modifierExecutionCount = 0;
		voxel::Region regions[2];
		modifier.aabbAction(&volume, [&] (const voxel::Region &region, ModifierType modifierType) {
			if (modifierExecutionCount < lengthof(regions)) {
				regions[modifierExecutionCount] = region;
			}
			++modifierExecutionCount;
		});
		EXPECT_EQ(lengthof(regions), modifierExecutionCount);
		EXPECT_EQ(voxel::Region(regMins, regMaxs), regions[0]) << regions[0];
		EXPECT_EQ(voxel::Region(expectedMins, expectedMaxs), regions[1]) << regions[1];
		EXPECT_NE(voxel::Voxel(), modifier.cursorVoxel());
		EXPECT_EQ(modifier.cursorVoxel(), volume.voxel(regMins));
		modifier.shutdown();
	}
};

TEST_F(ModifierTest, testModifierStartStop) {
	Modifier modifier;
	ASSERT_TRUE(modifier.init());
	EXPECT_TRUE(modifier.aabbStart());
	EXPECT_TRUE(modifier.aabbMode());
	modifier.aabbStop();
	EXPECT_FALSE(modifier.aabbMode());
	modifier.shutdown();
}

TEST_F(ModifierTest, testModifierDim) {
	Modifier modifier;
	ASSERT_TRUE(modifier.init());
	prepare(modifier, glm::ivec3(-1), glm::ivec3(1), ModifierType::Place);
	const glm::ivec3& dim = modifier.aabbDim();
	EXPECT_EQ(glm::ivec3(3), dim);
	modifier.shutdown();
}

TEST_F(ModifierTest, testModifierAction) {
	Modifier modifier;
	ASSERT_TRUE(modifier.init());
	prepare(modifier, glm::ivec3(-1), glm::ivec3(1), ModifierType::Place);
	const voxel::Region region(-10, 10);
	voxel::RawVolume volume(region);
	bool modifierExecuted = false;
	modifier.aabbAction(&volume, [&] (const voxel::Region &region, ModifierType modifierType) {
		modifierExecuted = true;
		EXPECT_EQ(voxel::Region(glm::ivec3(-1), glm::ivec3(1)), region);
	});
	EXPECT_TRUE(modifierExecuted);
	modifier.shutdown();
}

TEST_F(ModifierTest, testModifierActionMirrorAxisX) {
	testMirror(math::Axis::X, glm::ivec3(-2), glm::ivec3(-2, -2, 1));
}

TEST_F(ModifierTest, testModifierActionMirrorAxisY) {
	testMirror(math::Axis::Y, glm::ivec3(-2), glm::ivec3(-2, 1, -2));
}

TEST_F(ModifierTest, testModifierActionMirrorAxisZ) {
	testMirror(math::Axis::Z, glm::ivec3(-2), glm::ivec3(1, -2, -2));
}

}
