/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "util/IncludeUtil.h"

class IncludeUtilTest : public core::AbstractTest {
};

TEST_F(IncludeUtilTest, testInclude) {
	std::vector<std::string> includedFiles;
	std::vector<std::string> includeDirs { "." };
	const std::string src = io::filesystem()->load("main.h");
	EXPECT_EQ(34u, src.size());
	std::pair<std::string, bool> retIncludes = util::handleIncludes(src, includeDirs, &includedFiles);
	EXPECT_TRUE(retIncludes.second);
	EXPECT_EQ(2u, includedFiles.size());
	EXPECT_EQ("#error \"one\"\n#include \"two.h\"\n\n#error \"two\"\n\n", retIncludes.first);
	retIncludes = util::handleIncludes(retIncludes.first, includeDirs, &includedFiles);
	EXPECT_TRUE(retIncludes.second);
	EXPECT_EQ(3u, includedFiles.size());
	EXPECT_EQ("#error \"one\"\n#error \"two\"\n\n\n#error \"two\"\n\n", retIncludes.first);
}
