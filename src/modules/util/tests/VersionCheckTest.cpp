/**
 * @file
 */

#include "util/VersionCheck.h"
#include "app/tests/AbstractTest.h"

class VersionCheckTest : public app::AbstractTest {};

TEST_F(VersionCheckTest, testReleaseResponse) {
	EXPECT_TRUE(util::isNewVersionAvailable(R"({"tag_name":"v9999.0.0"})"));
	EXPECT_FALSE(util::isNewVersionAvailable(R"({"tag_name":"v0.0.0"})"));
	EXPECT_FALSE(util::isNewVersionAvailable(R"({"tag_name":""})"));
	EXPECT_FALSE(util::isNewVersionAvailable(R"({"message":"rate limited"})"));
	EXPECT_FALSE(util::isNewVersionAvailable("invalid json"));
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
