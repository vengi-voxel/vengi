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
	ASSERT_EQ(c.size(), 4u);
	EXPECT_EQ(c[0], "C:");
	EXPECT_EQ(c[1], "Program Files");
	EXPECT_EQ(c[2], "Foo");
	EXPECT_EQ(c[3], "Baz");
}

TEST_F(PathTest, testOperators) {
	Path path1("C:\\Program Files\\Foo\\Baz");
	Path path2("C:\\Program Files\\Foo\\Baz\\");
	EXPECT_EQ(path1, path2);
}

TEST_F(PathTest, testHasParent) {
	Path path1("C:\\Program Files\\Foo\\Baz\\");
	EXPECT_TRUE(path1.hasParentDirectory());
	Path path2("C:\\Program Files\\Foo\\Baz");
	EXPECT_TRUE(path2.hasParentDirectory());
	Path path3("C:\\Program Files");
	EXPECT_TRUE(path3.hasParentDirectory());
	Path path4("C:\\");
	EXPECT_FALSE(path4.hasParentDirectory());
	Path path5("/");
	EXPECT_FALSE(path5.hasParentDirectory());
	Path path6("foo");
	EXPECT_FALSE(path6.hasParentDirectory());
	Path path7("foo/bar");
	EXPECT_TRUE(path7.hasParentDirectory());
}

TEST_F(PathTest, testLexicallyNormal) {
	core::String path1 = "/foo/././bar";
	core::String path2 = "/foo/../bar";
	core::String path3 = "./../foo/bar";
	core::String path4 = "/../..";

	Path p1(path1);
	EXPECT_EQ(p1.lexicallyNormal(), "/foo/bar") << p1.lexicallyNormal();
	Path p2(path2);
	EXPECT_EQ(p2.lexicallyNormal(), "/bar") << p2.lexicallyNormal();
	Path p3(path3);
	EXPECT_EQ(p3.lexicallyNormal(), "../foo/bar") << p3.lexicallyNormal();
	Path p4(path4);
	EXPECT_EQ(p4.lexicallyNormal(), "/") << p4.lexicallyNormal();

	Path pathWindows("C:\\Program Files");
	EXPECT_EQ(pathWindows.lexicallyNormal(), "C:/Program Files") << pathWindows.lexicallyNormal();

	Path path5("C:\\Program Files\\..\\.");
	EXPECT_EQ(path5.lexicallyNormal(), "C:/") << path5.lexicallyNormal();
	Path path6("C:\\foo\\bar\\..\\baz");
	EXPECT_EQ(path6.lexicallyNormal(), "C:/foo/baz") << path6.lexicallyNormal();
	Path path7("..\\foo\\bar");
	EXPECT_EQ(path7.lexicallyNormal(), "../foo/bar") << path7.lexicallyNormal();

	Path path8("/foo///././//bar//\\");
	EXPECT_EQ(path8.lexicallyNormal(), "/foo/bar") << path8.lexicallyNormal();
}

TEST_F(PathTest, testPopFront) {
	Path path1("C:\\Program Files\\Foo\\Baz");
	EXPECT_EQ(path1.popFront(), "Program Files/Foo/Baz");
	Path path2("Program Files\\Foo\\Baz");
	EXPECT_EQ(path2.popFront(), "Foo/Baz");
	Path path3("Foo\\Baz");
	EXPECT_EQ(path3.popFront(), "Baz");
	Path path4("Baz");
	EXPECT_EQ(path4.popFront(), "");
	Path path5("/non-existing/cube/Cube_BaseColor.png");
	EXPECT_EQ(path5.popFront(), "non-existing/cube/Cube_BaseColor.png");
}

TEST_F(PathTest, testPopBack) {
	Path path0("C:\\Program Files\\Foo\\Baz\\");
	EXPECT_EQ(path0.lexicallyNormal(), "C:/Program Files/Foo/Baz");
	EXPECT_EQ(path0.popBack(), "C:/Program Files/Foo");
	Path path1("C:\\Program Files\\Foo\\Baz");
	EXPECT_EQ(path1.popBack(), "C:/Program Files/Foo");
	Path path2("Program Files\\Foo\\Baz");
	EXPECT_EQ(path2.popBack(), "Program Files/Foo");
	Path path3("Foo\\Baz");
	EXPECT_EQ(path3.popBack(), "Foo");
	Path path4("Baz");
	EXPECT_EQ(path4.popBack(), "");
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
