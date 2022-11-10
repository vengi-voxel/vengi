/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "util/IncludeUtil.h"
#include "io/Filesystem.h"

class IncludeUtilTest : public app::AbstractTest {
};

TEST_F(IncludeUtilTest, testInclude) {
	core::List<core::String> includedFiles;
	core::List<core::String> includeDirs { "." };
	const core::String src = io::filesystem()->load("main.h");
	EXPECT_FALSE(src.empty());
	core::Pair<core::String, bool> retIncludes = util::handleIncludes("originalfile", src, includeDirs, &includedFiles);
	EXPECT_TRUE(retIncludes.second);
	EXPECT_EQ(2u, includedFiles.size());
	EXPECT_EQ("#error \"one\"\n#include \"two.h\"\n\n#error \"two\"\n\n", retIncludes.first);
	retIncludes = util::handleIncludes("originalfile", retIncludes.first, includeDirs, &includedFiles);
	EXPECT_TRUE(retIncludes.second);
	EXPECT_EQ(3u, includedFiles.size());
	EXPECT_EQ("#error \"one\"\n#error \"two\"\n\n\n#error \"two\"\n\n", retIncludes.first);
}
