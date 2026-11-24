/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "util/XMLUtil.h"

class XMLUtilTest : public app::AbstractTest {};

TEST_F(XMLUtilTest, testXmlEscape) {
	EXPECT_TRUE(util::xmlEscape(nullptr).empty());
	EXPECT_EQ("abc", util::xmlEscape("abc"));
	EXPECT_EQ("a&amp;b&lt;c&gt;d&quot;e&apos;f", util::xmlEscape("a&b<c>d\"e'f"));
}
