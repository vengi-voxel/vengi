/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "io/BrotliReadStream.h"
#include "io/BufferedReadWriteStream.h"
#include "core/ArrayLength.h"
#include <gtest/gtest.h>

namespace io {

class BrotliReadStreamTest : public app::AbstractTest {};

// brotli-compressed "hello vengi"
static const uint8_t helloVengi[] = {0x0b, 0x05, 0x80, 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x76, 0x65, 0x6e, 0x67, 0x69, 0x03};

// brotli-compressed empty payload
static const uint8_t emptyCompressed[] = {0x06};

// brotli-compressed 2048 bytes of 'A'
static const uint8_t repeats[] = {0x1b, 0xff, 0x07, 0xf8, 0x25, 0x82, 0xc2, 0xb1, 0x40, 0x20, 0x77};

TEST_F(BrotliReadStreamTest, testHelloVengi) {
	BufferedReadWriteStream compressed;
	ASSERT_EQ((int)sizeof(helloVengi), compressed.write(helloVengi, sizeof(helloVengi)));
	compressed.seek(0);
	BrotliReadStream stream(compressed, (int)sizeof(helloVengi));
	char buf[16]{};
	ASSERT_EQ(11, stream.read(buf, sizeof(buf)));
	ASSERT_STREQ("hello vengi", buf);
	ASSERT_TRUE(stream.eos());
	ASSERT_EQ(0, stream.read(buf, sizeof(buf)));
}

TEST_F(BrotliReadStreamTest, testEmpty) {
	BufferedReadWriteStream compressed;
	ASSERT_EQ((int)sizeof(emptyCompressed), compressed.write(emptyCompressed, sizeof(emptyCompressed)));
	compressed.seek(0);
	BrotliReadStream stream(compressed);
	ASSERT_EQ(0, stream.size());
	uint8_t byte;
	ASSERT_EQ(0, stream.read(&byte, 1));
}

TEST_F(BrotliReadStreamTest, testRepeats) {
	BufferedReadWriteStream compressed;
	ASSERT_EQ((int)sizeof(repeats), compressed.write(repeats, sizeof(repeats)));
	compressed.seek(0);
	BrotliReadStream stream(compressed);
	ASSERT_EQ(2048, stream.size());
	uint8_t buf[2048];
	ASSERT_EQ(2048, stream.read(buf, sizeof(buf)));
	for (int i = 0; i < lengthof(buf); ++i) {
		ASSERT_EQ('A', buf[i]) << "mismatch at " << i;
	}
	ASSERT_TRUE(stream.eos());
}

TEST_F(BrotliReadStreamTest, testSeek) {
	BufferedReadWriteStream compressed;
	ASSERT_EQ((int)sizeof(helloVengi), compressed.write(helloVengi, sizeof(helloVengi)));
	compressed.seek(0);
	BrotliReadStream stream(compressed);
	ASSERT_EQ(6, stream.seek(6));
	char buf[16]{};
	ASSERT_EQ(5, stream.read(buf, sizeof(buf)));
	ASSERT_STREQ("vengi", buf);
	ASSERT_EQ(0, stream.seek(0));
	ASSERT_EQ(11, stream.read(buf, sizeof(buf)));
	ASSERT_STREQ("hello vengi", buf);
}

TEST_F(BrotliReadStreamTest, testInvalid) {
	const uint8_t invalid[] = {0xff, 0xff, 0xff, 0xff};
	BufferedReadWriteStream compressed;
	ASSERT_EQ((int)sizeof(invalid), compressed.write(invalid, sizeof(invalid)));
	compressed.seek(0);
	BrotliReadStream stream(compressed);
	uint8_t byte;
	ASSERT_EQ(-1, stream.read(&byte, 1));
}

} // namespace io
