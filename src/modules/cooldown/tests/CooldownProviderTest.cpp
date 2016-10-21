/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "cooldown/CooldownProvider.h"

namespace cooldown {

class CooldownDurationTest : public core::AbstractTest {
};

TEST_F(CooldownDurationTest, testLoading) {
	CooldownProvider d;
	ASSERT_TRUE(d.init("cooldowns.lua")) << d.error();
	ASSERT_NE(DefaultDuration, d.duration(Type::LOGOUT));
}

}
