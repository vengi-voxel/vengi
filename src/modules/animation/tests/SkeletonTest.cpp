/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "animation/chr/CharacterSkeleton.h"
#include "animation/AnimationSettings.h"
#include "animation/BoneId.h"
#include "core/io/Filesystem.h"

namespace animation {

class SkeletonTest: public core::AbstractTest {
protected:
	void test(const char *file) {
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
	}
};

TEST_F(SkeletonTest, testHumanMaleKnight) {
	test("chr/human-male-knight.lua");
}

TEST_F(SkeletonTest, testHumanMaleBlacksmith) {
	test("chr/human-male-blacksmith.lua");
}

}
