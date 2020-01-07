/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "util/IncludeUtil.h"
#include <SDL_platform.h>

#ifdef __WINDOWS__
#define NEWLINE "\r\n"
#else
#define NEWLINE "\n"
#endif

class IncludeUtilTest : public core::AbstractTest {
};

TEST_F(IncludeUtilTest, testInclude) {
	std::vector<std::string> includedFiles;
	std::vector<std::string> includeDirs { "." };
	const std::string src = io::filesystem()->load("main.h");
	EXPECT_FALSE(src.empty());
	std::pair<std::string, bool> retIncludes = util::handleIncludes(src, includeDirs, &includedFiles);
	EXPECT_TRUE(retIncludes.second);
	EXPECT_EQ(2u, includedFiles.size());
	EXPECT_EQ("#error \"one\"" NEWLINE "#include \"two.h\"" NEWLINE NEWLINE "#error \"two\"" NEWLINE NEWLINE, retIncludes.first);
	retIncludes = util::handleIncludes(retIncludes.first, includeDirs, &includedFiles);
	EXPECT_TRUE(retIncludes.second);
	EXPECT_EQ(3u, includedFiles.size());
	EXPECT_EQ("#error \"one\"" NEWLINE "#error \"two\"" NEWLINE NEWLINE NEWLINE "#error \"two\"" NEWLINE NEWLINE, retIncludes.first);
}
