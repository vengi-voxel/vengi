/**
 * @file
 */

#include "animation/AnimationSystem.h"
#include "app/tests/AbstractTest.h"
#include "animation/chr/CharacterSkeleton.h"
#include "animation/AnimationSettings.h"
#include "animation/BoneId.h"
#include "io/Filesystem.h"

namespace animation {

class SkeletonTest: public app::AbstractTest {
protected:
	void test(const char *file) {
		AnimationSystem system;
		ASSERT_TRUE(system.init());
		CharacterSkeleton skel;
		AnimationSettings settings;
		const core::String& lua = io::filesystem()->load("%s", file);
		ASSERT_TRUE(loadAnimationSettings(lua, settings, nullptr));
		glm::mat4 bones[shader::SkeletonShaderConstants::getMaxBones()];
		skel.update(settings, bones);
		EXPECT_NE(-1, settings.mapBoneIdToArrayIndex(BoneId::Head));
		EXPECT_NE(-1, settings.mapBoneIdToArrayIndex(BoneId::Chest));
		EXPECT_NE(-1, settings.mapBoneIdToArrayIndex(BoneId::Belt));
		EXPECT_NE(-1, settings.mapBoneIdToArrayIndex(BoneId::Pants));
		EXPECT_NE(-1, settings.mapBoneIdToArrayIndex(BoneId::LeftHand));
		EXPECT_NE(-1, settings.mapBoneIdToArrayIndex(BoneId::RightHand));
		EXPECT_NE(-1, settings.mapBoneIdToArrayIndex(BoneId::Tool));
		EXPECT_NE(-1, settings.mapBoneIdToArrayIndex(BoneId::LeftShoulder));
		EXPECT_NE(-1, settings.mapBoneIdToArrayIndex(BoneId::RightShoulder));
		EXPECT_NE(-1, settings.mapBoneIdToArrayIndex(BoneId::Glider));
		system.shutdown();
	}
};

TEST_F(SkeletonTest, testHumanMaleKnight) {
	test("chr/human-male-knight.lua");
}

TEST_F(SkeletonTest, testHumanMaleBlacksmith) {
	test("chr/human-male-blacksmith.lua");
}

}
