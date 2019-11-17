/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "../anim/AnimationLuaSaver.h"
#include "core/String.h"

namespace voxedit {

class AnimationLuaSaverTest: public core::AbstractTest {
};

TEST_F(AnimationLuaSaverTest, testSaveDefaultValues) {
	const io::FilePtr& file = io::filesystem()->open("testSaveDefaultValues.lua", io::FileMode::Write);
	animation::CharacterSettings settings;
	ASSERT_TRUE(saveCharacterLua(settings, "foo", file));
}

TEST_F(AnimationLuaSaverTest, testSave) {
	const io::FilePtr& file = io::filesystem()->open("testSave.lua", io::FileMode::Write);
	animation::CharacterSettings settings;
	settings.skeletonAttr.neckHeight = -1337.0f;
	ASSERT_TRUE(saveCharacterLua(settings, "foo", file));
	ASSERT_TRUE(core::string::contains(io::filesystem()->load("testSave.lua"), "-1337.0"));
}

}
