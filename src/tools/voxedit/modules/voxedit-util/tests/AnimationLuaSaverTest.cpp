/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "../anim/AnimationLuaSaver.h"
#include "core/StringUtil.h"
#include "core/io/Filesystem.h"

namespace voxedit {

class AnimationLuaSaverTest: public core::AbstractTest {
};

TEST_F(AnimationLuaSaverTest, testSaveDefaultValues) {
	const io::FilePtr& file = io::filesystem()->open("testSaveDefaultValues.lua", io::FileMode::Write);
	EXPECT_TRUE(file && file->validHandle());
	animation::AnimationSettings settings;
	animation::CharacterSkeletonAttribute attributes;
	ASSERT_TRUE(saveAnimationEntityLua(settings, attributes, "foo", file));
}

TEST_F(AnimationLuaSaverTest, testSave) {
	const io::FilePtr& file = io::filesystem()->open("testSave.lua", io::FileMode::Write);
	EXPECT_TRUE(file && file->validHandle());
	animation::AnimationSettings settings;
	animation::CharacterSkeletonAttribute attributes;
	attributes.neckHeight = -1337.0f;
	ASSERT_TRUE(saveAnimationEntityLua(settings, attributes, "foo", file));
	file->close(); // flush
	const io::FilePtr& fileForRead = io::filesystem()->open("testSave.lua");
	EXPECT_STREQ(fileForRead->name().c_str(), file->name().c_str());
	const core::String& luaContent = fileForRead->load();
	EXPECT_NE(luaContent, "");
	EXPECT_TRUE(core::string::contains(luaContent, "-1337.0")) << luaContent;
}

}
