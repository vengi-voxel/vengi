/**
 * @file
 */

#include "AbstractTest.h"
#include "core/String.h"

namespace core {

class StringTest: public AbstractTest {
};

TEST_F(StringTest, testEraseAllSpaces) {
	ASSERT_EQ("", core::string::eraseAllSpaces("   "));
	ASSERT_EQ(",", core::string::eraseAllSpaces("  ,  "));
}

TEST_F(StringTest, testExtractFilename) {
	ASSERT_EQ("file", core::string::extractFilename("/path/to/file.extension"));
	ASSERT_EQ("file", core::string::extractFilename("file.extension"));
	ASSERT_EQ("file", core::string::extractFilename("/file.extension"));
	ASSERT_EQ("file", core::string::extractFilename("file"));
}

TEST_F(StringTest, testCutAfterFirstMatch) {
	std::string_view test("filename.ext");
	ASSERT_EQ("filename", core::string::cutAfterFirstMatch(test, "."));
}

TEST_F(StringTest, testCutAfterFirstMatchString) {
	std::string test("filename.ext");
	ASSERT_EQ("filename", core::string::cutAfterFirstMatch(test, "."));
}

TEST_F(StringTest, testToLower) {
	std::string test("FILENAME.EXT");
	ASSERT_EQ("filename.ext", core::string::toLower(test));
}

}
