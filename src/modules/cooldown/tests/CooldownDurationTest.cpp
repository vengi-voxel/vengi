/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "cooldown/CooldownDuration.h"

namespace cooldown {

class CooldownDurationTest : public core::AbstractTest {
};

TEST_F(CooldownDurationTest, testLoading) {
	CooldownDuration d;
	ASSERT_TRUE(d.init("cooldowns.lua")) << d.error();
	ASSERT_NE(DefaultDuration, d.duration(Type::LOGOUT));
}

}
