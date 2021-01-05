/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "command/CommandCompleter.h"
#include "io/Filesystem.h"
#include "core/tests/TestHelper.h"

namespace app {

class CommandCompleterTest: public app::AbstractTest {
public:
	bool onInitApp() override{
		const io::FilesystemPtr& fs = io::filesystem();
		fs->createDir("commandcompletertest/dir1");
		fs->syswrite("commandcompletertest/dir1/ignored", "ignore");
		fs->syswrite("commandcompletertest/dir1/ignoredtoo", "ignore");
		fs->syswrite("commandcompletertest/dir1/foo1.foo", "foo1");
		fs->syswrite("commandcompletertest/file1", "1");
		fs->syswrite("commandcompletertest/file2", "2");
		fs->syswrite("commandcompletertest/foo1.foo", "foo1");
		fs->syswrite("commandcompletertest/foo2.foo", "foo2");
		fs->syswrite("commandcompletertest/foo3.foo", "foo3");
		fs->syswrite("commandcompletertest/foo4.foo", "foo4");
		return AbstractTest::onInitApp();
	}
};

TEST_F(CommandCompleterTest, testComplete) {
	auto func = command::fileCompleter(io::filesystem(), "commandcompletertest/", "*.foo");
	core::DynamicArray<core::String> matches;
	const int matchCount = func("", matches);
	ASSERT_GE(matchCount, 5) << toString(matches);
	EXPECT_EQ("dir1/", matches[0]) << toString(matches);
	EXPECT_EQ("foo1.foo", matches[1]) << toString(matches);
	EXPECT_EQ("foo2.foo", matches[2]) << toString(matches);
	EXPECT_EQ("foo3.foo", matches[3]) << toString(matches);
	EXPECT_EQ("foo4.foo", matches[4]) << toString(matches);
}

TEST_F(CommandCompleterTest, testCompleteOnlyFiles) {
	auto func = command::fileCompleter(io::filesystem(), "commandcompletertest/", "*.foo");
	core::DynamicArray<core::String> matches;
	const int matchCount = func("f", matches);
	ASSERT_GE(matchCount, 4) << toString(matches);
	EXPECT_EQ("foo1.foo", matches[0]) << toString(matches);
	EXPECT_EQ("foo2.foo", matches[1]) << toString(matches);
	EXPECT_EQ("foo3.foo", matches[2]) << toString(matches);
	EXPECT_EQ("foo4.foo", matches[3]) << toString(matches);
}

TEST_F(CommandCompleterTest, testCompleteSubdir) {
	auto func = command::fileCompleter(io::filesystem(), "commandcompletertest/", "*.foo");
	core::DynamicArray<core::String> matches;
	const int matchCount = func("dir1", matches);
	ASSERT_GE(matchCount, 1) << toString(matches);
	EXPECT_EQ("dir1/", matches[0]) << toString(matches);
}

TEST_F(CommandCompleterTest, testCompleteSubdirFile) {
	auto func = command::fileCompleter(io::filesystem(), "commandcompletertest/dir1/", "*.foo");
	core::DynamicArray<core::String> matches;
	const int matchCount = func("f", matches);
	ASSERT_GE(matchCount, 1) << toString(matches);
	EXPECT_EQ("foo1.foo", matches[0]) << toString(matches);
}

TEST_F(CommandCompleterTest, testCompleteSubdirFile2) {
	auto func = command::fileCompleter(io::filesystem(), "commandcompletertest/", "*.foo");
	core::DynamicArray<core::String> matches;
	const int matchCount = func("dir1/f", matches);
	ASSERT_GE(matchCount, 1) << toString(matches);
	EXPECT_EQ("dir1/foo1.foo", matches[0]) << toString(matches);
}

}
