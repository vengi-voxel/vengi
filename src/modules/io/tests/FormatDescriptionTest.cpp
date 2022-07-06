#include <gtest/gtest.h>
#include "io/FormatDescription.h"

namespace io {

class FormatDescriptionTest: public testing::Test {
};

TEST_F(FormatDescriptionTest, testGetPath) {
	ASSERT_EQ(core::String("*.foo"), getWildcardsFromPattern("Foo (*.foo)"));
	ASSERT_EQ(core::String("*.foo,*.bar"), getWildcardsFromPattern("Foo (*.foo,*.bar)"));
}

TEST_F(FormatDescriptionTest, testIsImage) {
	ASSERT_TRUE(isImage("foobar.PNG"));
	ASSERT_TRUE(isImage("foobar.png"));
	ASSERT_FALSE(isImage("foobar.foo"));
}

TEST_F(FormatDescriptionTest, testConvertToAllFilePattern) {
	const FormatDescription desc[] = {
		{"Portable Network Graphics", {"png"}, nullptr, 0u},
		{"JPEG", {"jpeg", "jpg"}, nullptr, 0u},
		{"Portable Anymap", {"pnm"}, nullptr, 0u},
		{"", {}, nullptr, 0u}
	};
	const core::String &all = convertToAllFilePattern(desc);
	ASSERT_EQ("*.png,*.jpeg,*.jpg,*.pnm", all);
}

TEST_F(FormatDescriptionTest, testConvertToFilePattern) {
	const FormatDescription desc1 = {"Name", {"ext1"}, nullptr, 0u};
	const FormatDescription desc2 = {"Name", {"ext1", "ext2"}, nullptr, 0u};

	ASSERT_EQ("Name (*.ext1)", convertToFilePattern(desc1));
	ASSERT_EQ("Name (*.ext1,*.ext2)", convertToFilePattern(desc2));
}

}
