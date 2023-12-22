/**
 * @file
 */

#include "io/Base64.h"
#include "io/BufferedReadWriteStream.h"
#include <gtest/gtest.h>

namespace io {

TEST(Base64Test, testBase64Encode) {
	const char *foobar = "foobar";
	const core::String &base64 = io::Base64::encode((const uint8_t *)foobar, strlen(foobar));
	EXPECT_STREQ("Zm9vYmFy", base64.c_str());
}

TEST(Base64Test, testBase64Decode) {
	const core::String input = "Zm9vYmFy";
	io::BufferedReadWriteStream stream;
	EXPECT_TRUE(io::Base64::decode(stream, input));
	EXPECT_EQ(6u, stream.size());

	char strbuff[7];
	stream.seek(0);
	EXPECT_TRUE(stream.readString(sizeof(strbuff) - 1, strbuff, false));
	strbuff[6] = '\0';
	EXPECT_STREQ("foobar", strbuff);
}

} // namespace util
