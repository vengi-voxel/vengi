/**
 * @file
 */

#include "core/tests/TestHelper.h"
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
		base64Stream.readString(expectedOutput.size(), out, false);
		EXPECT_EQ(out, expectedOutput);
	}

	void encode(const core::String &input, const core::String &expectedBase64Output, bool flush = true) {
		io::BufferedReadWriteStream stream;
		io::Base64WriteStream base64Stream(stream);
		base64Stream.writeString(input, false);
		if (flush) {
			base64Stream.flush();
		}
		stream.seek(0);
		core::String base64;
		EXPECT_EQ(expectedBase64Output.size(), stream.size());
		stream.readString(stream.size(), base64, false);
		EXPECT_EQ(expectedBase64Output, base64);
	}
};

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
	encode("d", "ZA==");
	encode("z", "eg==");
	encode("fo", "Zm8=");
	encode("foobar", "Zm9vYmFy");
}

TEST_F(Base64Test, testBase64ReadStream) {
	decode("ZA==", "d");
	decode("eg==", "z");
	decode("Zm8=", "fo");
	decode("Zm9vYmFy", "foobar");
}

} // namespace io
