/**
 * @file
 */

#include "io/Base64.h"
#include "core/tests/TestHelper.h"
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

		EXPECT_EQ(0u, base64Input.size() % 4) << "Unexpected input size: " << base64Input.size() << " for " << base64Input;

		core::String out;
		base64Stream.readString((int)expectedOutput.size(), out, false);
		ASSERT_EQ(out, expectedOutput);
		ASSERT_TRUE(stream.eos()) << "Still " << stream.remaining() << " bytes left in the stream of size "
								  << stream.size() << " at pos " << stream.pos();
		EXPECT_TRUE(base64Stream.eos());
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
		EXPECT_EQ(expectedBase64Output.size(), (size_t)stream.size());
		stream.readString((int)stream.size(), base64, false);
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
	EXPECT_TRUE(stream.readString((int)(sizeof(strbuff) - 1), strbuff, false));
	strbuff[6] = '\0';
	EXPECT_STREQ("foobar", strbuff);
}

TEST_F(Base64Test, testBase64WriteStream) {
	encode("d", "ZA==");
	encode("z", "eg==");
	encode("fo", "Zm8=");
	encode("foo", "Zm9v");
	encode("foobar", "Zm9vYmFy");
}

TEST_F(Base64Test, testBase64ReadStream) {
	decode("ZA==", "d");
	decode("eg==", "z");
	decode("Zm8=", "fo");
	decode("Zm9v", "foo");
	decode("Zm9vYmFy", "foobar");
}

TEST_F(Base64Test, roundTrip) {
	core::DynamicArray<uint32_t> data;
	data.reserve(1024 * 1024 * 3);

	io::BufferedReadWriteStream outStream(data.capacity() * 4);
	Base64WriteStream stream(outStream);
	ASSERT_NE(stream.write(data.data(), data.capacity()), -1);
	ASSERT_TRUE(stream.flush());
	outStream.seek(0);
	Base64ReadStream inStream(outStream);
	core::DynamicArray<uint32_t> decoded;
	decoded.resize(data.capacity());
	inStream.read(decoded.data(), decoded.size() * sizeof(uint32_t));
};

} // namespace io
