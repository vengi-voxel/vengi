/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "cooldown/CooldownProvider.h"
#include "io/Filesystem.h"

namespace cooldown {

class CooldownDurationTest : public app::AbstractTest {
};

TEST_F(CooldownDurationTest, testLoading) {
	CooldownProvider d;
	const core::String& cooldowns = io::filesystem()->load("cooldowns.lua");
	ASSERT_TRUE(d.init(cooldowns)) << d.error();
	ASSERT_NE(DefaultDuration, d.duration(Type::LOGOUT));
}

}
