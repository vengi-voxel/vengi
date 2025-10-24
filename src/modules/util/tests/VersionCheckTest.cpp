/**
 * @file
 */

#include "util/VersionCheck.h"
#include "app/tests/AbstractTest.h"

class VersionCheckTest : public app::AbstractTest {};

TEST_F(VersionCheckTest, DISABLED_testIsNewVersionAvailable) {
	ASSERT_FALSE(util::isNewVersionAvailable());
}

TEST_F(VersionCheckTest, testIsNewerVersion) {
	EXPECT_TRUE(util::isNewerVersion("2.0.0", "1.0.0"));
	EXPECT_FALSE(util::isNewerVersion("0.0.27", "0.0.28.0"));
	EXPECT_FALSE(util::isNewerVersion("1.0.0", "2.0.0"));
	EXPECT_FALSE(util::isNewerVersion("1.0.0.0", "1.0.0.0"));
	EXPECT_FALSE(util::isNewerVersion("invalidversion", "1.0.0.0"));
	EXPECT_TRUE(util::isNewerVersion("2.0.0.1", "2.0.0"));
	EXPECT_FALSE(util::isNewerVersion("2.0.0.0", "2.0.0.1"));
}
