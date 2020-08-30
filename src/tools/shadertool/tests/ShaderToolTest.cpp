/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "../Util.h"

class ShaderToolTest: public app::AbstractTest {
};

TEST_F(ShaderToolTest, testConvertName) {
	EXPECT_EQ("fooBar", util::convertName("foo_bar", false));
	EXPECT_EQ("FooBar", util::convertName("foo_bar", true));
}
