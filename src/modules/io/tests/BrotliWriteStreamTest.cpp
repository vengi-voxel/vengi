/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "io/BrotliReadStream.h"
#include "io/BrotliWriteStream.h"
#include "io/BufferedReadWriteStream.h"
#include <gtest/gtest.h>

namespace io {

class BrotliWriteStreamTest : public app::AbstractTest {};

TEST_F(BrotliWriteStreamTest, testRoundTrip) {
	const char *payload = "hello vengi";
	BufferedReadWriteStream compressed;
	{
		BrotliWriteStream encoder(compressed, 4);
		ASSERT_EQ(11, encoder.write(payload, 11));
		ASSERT_TRUE(encoder.flush());
	}
	ASSERT_GT(compressed.size(), 0);
	ASSERT_EQ(0, compressed.seek(0));
	BrotliReadStream decoder(compressed);
	char buf[16]{};
	ASSERT_EQ(11, decoder.read(buf, sizeof(buf)));
	ASSERT_STREQ(payload, buf);
	ASSERT_TRUE(decoder.eos());
}

TEST_F(BrotliWriteStreamTest, testEmptyFlush) {
	BufferedReadWriteStream compressed;
	{
		BrotliWriteStream encoder(compressed, 4);
		ASSERT_TRUE(encoder.flush());
	}
	ASSERT_GT(compressed.size(), 0);
	ASSERT_EQ(0, compressed.seek(0));
	BrotliReadStream decoder(compressed);
	ASSERT_EQ(0, decoder.size());
}

} // namespace io
