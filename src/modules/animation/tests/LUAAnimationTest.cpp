/**
 * @file
 */

#include "animation/SkeletonAttribute.h"
#include "animation/chr/CharacterSkeleton.h"
#include "animation/chr/CharacterSkeletonAttribute.h"
#include "app/App.h"
#include "app/tests/AbstractTest.h"
#include "animation/LUAAnimation.h"
#include "io/Filesystem.h"

namespace animation {

class LUAAnimationTest: public app::AbstractTest {
protected:
	void exec(const char *scriptName, const char *animation) {
		const core::String& script = io::filesystem()->load("%s", scriptName);
		lua::LUA lua;
		luaanim_setup(lua);
		ASSERT_TRUE(lua.load(script)) << lua.error();
		double animTime = 1.0;
		double velocity = 20.0;
		CharacterSkeleton skeleton;
		CharacterSkeletonAttribute attributes;
		attributes.init();
		ASSERT_TRUE(luaanim_execute(lua, animation, animTime, velocity, skeleton, attributes));
	}
};

TEST_F(LUAAnimationTest, testCharacterSwim) {
	exec("animations/character.lua", "swim");
}

}
