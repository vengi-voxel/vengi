/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "io/BufferedReadWriteStream.h"
#include "io/LZ4ReadStream.h"
#include "io/LZ4WriteStream.h"
#include <gtest/gtest.h>

namespace io {

class LZ4StreamTest : public app::AbstractTest {};

TEST_F(LZ4StreamTest, testRoundTrip) {
	const char *payload = "hello vengi";
	BufferedReadWriteStream compressed;
	{
		LZ4WriteStream encoder(compressed);
		ASSERT_EQ(11, encoder.write(payload, 11));
		ASSERT_TRUE(encoder.flush());
	}
	ASSERT_GT(compressed.size(), 0);
	ASSERT_EQ(0, compressed.seek(0));
	LZ4ReadStream decoder(compressed);
	char buf[16]{};
	ASSERT_EQ(11, decoder.read(buf, sizeof(buf)));
	ASSERT_STREQ(payload, buf);
	ASSERT_TRUE(decoder.eos());
}

TEST_F(LZ4StreamTest, testRoundTripLarge) {
	const core::String payload(65536, 'A');
	BufferedReadWriteStream compressed;
	{
		LZ4WriteStream encoder(compressed, 3);
		ASSERT_EQ((int)payload.size(), encoder.write(payload.c_str(), payload.size()));
		ASSERT_TRUE(encoder.flush());
	}
	ASSERT_LT(compressed.size(), (int64_t)payload.size());
	ASSERT_EQ(0, compressed.seek(0));
	LZ4ReadStream decoder(compressed);
	ASSERT_EQ((int64_t)payload.size(), decoder.size());
	core::String decoded(payload.size(), '\0');
	ASSERT_EQ((int)payload.size(), decoder.read(decoded.c_str(), payload.size()));
	ASSERT_EQ(payload, decoded);
	ASSERT_TRUE(decoder.eos());
}

TEST_F(LZ4StreamTest, testEmptyFlush) {
	BufferedReadWriteStream compressed;
	{
		LZ4WriteStream encoder(compressed);
		ASSERT_TRUE(encoder.flush());
	}
	ASSERT_GT(compressed.size(), 0);
	ASSERT_EQ(0, compressed.seek(0));
	LZ4ReadStream decoder(compressed);
	ASSERT_EQ(0, decoder.size());
}

TEST_F(LZ4StreamTest, testSeek) {
	const char *payload = "hello vengi";
	BufferedReadWriteStream compressed;
	{
		LZ4WriteStream encoder(compressed);
		ASSERT_EQ(11, encoder.write(payload, 11));
		ASSERT_TRUE(encoder.flush());
	}
	ASSERT_EQ(0, compressed.seek(0));
	LZ4ReadStream stream(compressed);
	ASSERT_EQ(6, stream.seek(6));
	char buf[16]{};
	ASSERT_EQ(5, stream.read(buf, sizeof(buf)));
	ASSERT_STREQ("vengi", buf);
	ASSERT_EQ(0, stream.seek(0));
	ASSERT_EQ(11, stream.read(buf, sizeof(buf)));
	ASSERT_STREQ(payload, buf);
}

TEST_F(LZ4StreamTest, testInvalid) {
	const uint8_t invalid[] = {0xff, 0xff, 0xff, 0xff};
	BufferedReadWriteStream compressed;
	ASSERT_EQ((int)sizeof(invalid), compressed.write(invalid, sizeof(invalid)));
	compressed.seek(0);
	LZ4ReadStream stream(compressed);
	uint8_t byte;
	ASSERT_EQ(-1, stream.read(&byte, 1));
}

} // namespace io
