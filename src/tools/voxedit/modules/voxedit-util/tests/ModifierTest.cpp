/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "../modifier/Modifier.h"
#include "voxel/Voxel.h"

namespace voxedit {

class ModifierTest: public app::AbstractTest {
private:
	using Super = app::AbstractTest;
protected:
	Modifier _modifier;

	void prepare(const glm::ivec3& mins, const glm::ivec3& maxs, ModifierType modifierType) {
		_modifier.setModifierType(modifierType);
		_modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
		_modifier.setGridResolution(1);
		_modifier.setCursorPosition(mins, voxel::FaceNames::PositiveX);
		EXPECT_TRUE(_modifier.aabbStart());
		EXPECT_TRUE(_modifier.aabbMode());
		_modifier.setCursorPosition(maxs, voxel::FaceNames::PositiveX);
		_modifier.aabbStep();
	}

public:
	void SetUp() override {
		Super::SetUp();
		ASSERT_TRUE(_modifier.init()) << "Initialization failed";
	}

	void TearDown() override {
		_modifier.shutdown();
		Super::TearDown();
	}
};

TEST_F(ModifierTest, testModifierStartStop) {
	EXPECT_TRUE(_modifier.aabbStart());
	EXPECT_TRUE(_modifier.aabbMode());
	_modifier.aabbStop();
	EXPECT_FALSE(_modifier.aabbMode());
}

TEST_F(ModifierTest, testModifierDim) {
	prepare(glm::ivec3(-1), glm::ivec3(1), ModifierType::Place);
	const glm::ivec3& dim = _modifier.aabbDim();
	EXPECT_EQ(glm::ivec3(3), dim);
}

TEST_F(ModifierTest, testModifierAction) {
	prepare(glm::ivec3(-1), glm::ivec3(1), ModifierType::Place);
	const voxel::Region region(-10, 10);
	voxel::RawVolume volume(region);
	bool modifierExecuted = false;
	_modifier.aabbAction(&volume, [&] (const voxel::Region &region, ModifierType modifierType) {
		modifierExecuted = true;
		EXPECT_EQ(voxel::Region(glm::ivec3(-1), glm::ivec3(1)), region);
	});
	EXPECT_TRUE(modifierExecuted);
}

}
