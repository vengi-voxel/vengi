/**
 * @file
 */

#include "core/Path.h"
#include "core/collection/DynamicArray.h"
#include "core/tests/TestHelper.h"

namespace core {

class PathTest : public testing::Test {};

TEST_F(PathTest, testPathLetter) {
	Path path1("C:\\Program Files");
	EXPECT_EQ(path1.driveLetter(), 'C');

	Path path2("C:\\");
	EXPECT_EQ(path2.driveLetter(), 'C');

	Path path3("C:");
	EXPECT_EQ(path3.driveLetter(), 'C');
}

TEST_F(PathTest, testComponents) {
	Path path1("C:\\Program Files\\Foo\\Baz");
	const core::DynamicArray<core::String> &c = path1.components();
	ASSERT_EQ(c.size(), 4);
	EXPECT_EQ(c[0], "C:");
	EXPECT_EQ(c[1], "Program Files");
	EXPECT_EQ(c[2], "Foo");
	EXPECT_EQ(c[3], "Baz");
}

TEST_F(PathTest, testWindows) {
	Path path1("C:\\Program Files\\");
	EXPECT_EQ(path1.driveLetter(), 'C');
	EXPECT_EQ(path1.dirname(), "C:");
	EXPECT_EQ(path1.basename(), "Program Files");
	EXPECT_EQ(path1.extension(), "");

	Path path2("C:\\Program Files\\foo.bar");
	EXPECT_EQ(path2.driveLetter(), 'C');
	EXPECT_EQ(path2.dirname(), "C:/Program Files");
	EXPECT_EQ(path2.basename(), "foo.bar");
	EXPECT_EQ(path2.extension(), "bar");

	Path path3("C:\\Program Files\\foo.bar\\");
	EXPECT_EQ(path3.driveLetter(), 'C');
	EXPECT_EQ(path3.dirname(), "C:/Program Files");
	EXPECT_EQ(path3.basename(), "foo.bar");
	EXPECT_EQ(path3.extension(), "");

	Path path4("C:\\Program Files\\foo.bar\\baz");
	EXPECT_EQ(path4.driveLetter(), 'C');
	EXPECT_EQ(path4.dirname(), "C:\\Program Files\\foo.bar");
	EXPECT_EQ(path4.basename(), "baz");
	EXPECT_EQ(path4.extension(), "");
}

TEST_F(PathTest, testPathDirname) {
	Path path1("/usr/local/bin");
	EXPECT_EQ(path1.dirname(), "/usr/local");

	Path path2("bin");
	EXPECT_EQ(path2.dirname(), ".");

	Path path3("");
	EXPECT_EQ(path3.dirname(), ".");

	Path path4(".");
	EXPECT_EQ(path4.dirname(), ".");

	Path path5("/");
	EXPECT_EQ(path5.dirname(), "/");

	Path path6("/usr/local/bin/");
	EXPECT_EQ(path6.dirname(), "/usr/local");
}

TEST_F(PathTest, testPathBasename) {
	Path path1("/usr/local/bin");
	EXPECT_EQ(path1.basename(), "bin");

	Path path2("/usr/local/");
	EXPECT_EQ(path2.basename(), "local");

	Path path3("/");
	EXPECT_EQ(path3.basename(), "/");

	Path path4("./");
	EXPECT_EQ(path4.basename(), ".");

	Path path5(".");
	EXPECT_EQ(path5.basename(), ".");

	Path path6("");
	EXPECT_EQ(path6.basename(), "");
}

TEST_F(PathTest, testPathExtension) {
	Path path1("/usr/local/bin.foo");
	EXPECT_EQ(path1.extension(), "foo");

	Path path2("/usr/local/");
	EXPECT_EQ(path2.extension(), "");
}

TEST_F(PathTest, testPathRemoveExtension) {
	Path path1("/usr/local/bin.foo");
	EXPECT_EQ(path1.removeExtension(), "/usr/local/bin");

	Path path2("/usr/local/");
	EXPECT_EQ(path2.removeExtension(), "/usr/local/");
}

TEST_F(PathTest, testPathReplaceExtension) {
	Path path1("/usr/local/bin.foo");
	EXPECT_EQ(path1.replaceExtension("bar"), "/usr/local/bin.bar");

	Path path2("/usr/local/");
	EXPECT_EQ(path2.replaceExtension("bar"), "/usr/local/.bar");
}

} // namespace core
