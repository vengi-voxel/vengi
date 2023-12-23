/**
 * @file
 */

#include "io/Base64.h"
#include "io/BufferedReadWriteStream.h"
#include "io/Base64WriteStream.h"
#include "io/Base64ReadStream.h"
#include "io/MemoryReadStream.h"
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

TEST(Base64Test, testBase64WriteStream) {
	const char *foobar = "foobar";
	io::BufferedReadWriteStream stream;
	Base64WriteStream base64Stream(stream);
	base64Stream.writeString(foobar);
	stream.seek(0);
	core::String base64;
	stream.readString(stream.size(), base64);
	EXPECT_STREQ("Zm9vYmFy", base64.c_str());
}

TEST(Base64Test, testBase64ReadStream) {
	const core::String input = "Zm9vYmFy";
	io::MemoryReadStream stream(input.c_str(), input.size());
	Base64ReadStream base64Stream(stream);

	core::String out;
	base64Stream.readString(6, out);
	EXPECT_EQ("foobar", out);
}

} // namespace util
