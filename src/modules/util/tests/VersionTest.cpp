/**
 * @file
 */

#include "util/Version.h"
#include "app/tests/AbstractTest.h"

class VersionTest : public app::AbstractTest {};

TEST_F(VersionTest, testVersion1) {
	util::Version v = util::parseVersion("4.1");
	EXPECT_EQ(v.majorVersion, 4);
	EXPECT_EQ(v.minorVersion, 1);

	v = util::parseVersion("14.21");
	EXPECT_EQ(v.majorVersion, 14);
	EXPECT_EQ(v.minorVersion, 21);
}
