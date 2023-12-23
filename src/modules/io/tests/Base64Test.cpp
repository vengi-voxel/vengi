/**
 * @file
 */

#include "io/Base64.h"
#include "io/Base64ReadStream.h"
#include "io/Base64WriteStream.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryReadStream.h"
#include <gtest/gtest.h>

namespace io {

class Base64Test : public testing::Test {
protected:
	void decode(const core::String &base64Input, const core::String &expectedOutput) {
		io::MemoryReadStream stream(base64Input.c_str(), base64Input.size());
		io::Base64ReadStream base64Stream(stream);

		core::String out;
		base64Stream.readString(expectedOutput.size(), out);
		EXPECT_EQ(out, expectedOutput);
	}

	void encode(const core::String &input, const core::String &expectedBase64Output) {
		io::BufferedReadWriteStream stream;
		io::Base64WriteStream base64Stream(stream);
		base64Stream.writeString(input);
		stream.seek(0);
		core::String base64;
		EXPECT_EQ(expectedBase64Output.size(), stream.size());
		stream.readString(stream.size(), base64);
		EXPECT_EQ(expectedBase64Output, base64);
	}
};

TEST_F(Base64Test, testBase64Encode) {
	const char *foobar = "foobar";
	const core::String &base64 = io::Base64::encode((const uint8_t *)foobar, strlen(foobar));
	EXPECT_STREQ("Zm9vYmFy", base64.c_str());
}

TEST_F(Base64Test, testBase64Decode) {
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

TEST_F(Base64Test, testBase64WriteStream) {
	encode("foobar", "Zm9vYmFy");
}

TEST_F(Base64Test, testBase64ReadStream) {
	decode("Zm9vYmFy", "foobar");
}

} // namespace io
