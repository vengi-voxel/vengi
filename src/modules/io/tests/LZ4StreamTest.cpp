/**
 * @file
 */

#include "engine-config.h" // USE_LZ4

#ifdef USE_LZ4

#include "app/tests/AbstractTest.h"
#include "io/BufferedReadWriteStream.h"
#include "io/LZ4ReadStream.h"
#include "io/LZ4WriteStream.h"
#include <gtest/gtest.h>

namespace io {

class LZ4StreamTest : public app::AbstractTest {};

TEST_F(LZ4StreamTest, testLZ4StreamWrite) {
	BufferedReadWriteStream stream(1024);
	{
		LZ4WriteStream w(stream);
		for (int i = 0; i < 64; ++i) {
			ASSERT_TRUE(w.writeInt32(i + 0)) << "unexpected write failure for step: " << i;
			ASSERT_TRUE(w.writeInt32(i + 1)) << "unexpected write failure for step: " << i;
			ASSERT_TRUE(w.writeInt32(i + 2)) << "unexpected write failure for step: " << i;
			ASSERT_TRUE(w.writeInt32(i + 3)) << "unexpected write failure for step: " << i;
		}
		ASSERT_TRUE(w.flush());
	}
}

TEST_F(LZ4StreamTest, testLZ4StreamWriteAndRead) {
	BufferedReadWriteStream stream(1024);
	{
		LZ4WriteStream w(stream);
		for (int i = 0; i < 64; ++i) {
			ASSERT_TRUE(w.writeInt32(i + 0)) << "unexpected write failure for step: " << i;
			ASSERT_TRUE(w.writeInt32(i + 1)) << "unexpected write failure for step: " << i;
			ASSERT_TRUE(w.writeInt32(i + 2)) << "unexpected write failure for step: " << i;
			ASSERT_TRUE(w.writeInt32(i + 3)) << "unexpected write failure for step: " << i;
		}
		ASSERT_TRUE(w.flush());
	}
	const int size = (int)stream.size();
	stream.seek(0);
	{
		LZ4ReadStream r(stream, size);
		for (int i = 0; i < 64; ++i) {
			int32_t n;
			ASSERT_EQ(0, r.readInt32(n)) << "unexpected read failure for step: " << i;
			ASSERT_EQ(n, i + 0) << "unexpected extracted value for step: " << i;
			ASSERT_EQ(0, r.readInt32(n)) << "unexpected read failure for step: " << i;
			ASSERT_EQ(n, i + 1) << "unexpected extracted value for step: " << i;
			ASSERT_EQ(0, r.readInt32(n)) << "unexpected read failure for step: " << i;
			ASSERT_EQ(n, i + 2) << "unexpected extracted value for step: " << i;
			ASSERT_EQ(0, r.readInt32(n)) << "unexpected read failure for step: " << i;
			ASSERT_EQ(n, i + 3) << "unexpected extracted value for step: " << i;
		}
	}
	stream.seek(0);
	{
		LZ4ReadStream r(stream, size);
		uint8_t buffer[64 * 4 * 4 + 10]; // bigger
		ASSERT_EQ(64 * 4 * 4, r.read(buffer, sizeof(buffer)));
		ASSERT_EQ(0, r.read(buffer, sizeof(buffer)));
		ASSERT_EQ(0, r.read(buffer, sizeof(buffer)));
	}
}

TEST_F(LZ4StreamTest, testLZ4StreamNoSize) {
	const int n = 64;
	const int size = n * 4 * sizeof(uint32_t);
	const int expectedLZ4pedSize = 285;
	BufferedReadWriteStream stream(size);
	{
		LZ4WriteStream w(stream);
		for (int i = 0; i < n; ++i) {
			ASSERT_TRUE(w.writeInt32(i + 0)) << "unexpected write failure for step: " << i;
			ASSERT_TRUE(w.writeInt32(i + 1)) << "unexpected write failure for step: " << i;
			ASSERT_TRUE(w.writeInt32(i + 2)) << "unexpected write failure for step: " << i;
			ASSERT_TRUE(w.writeInt32(i + 3)) << "unexpected write failure for step: " << i;
		}
		ASSERT_TRUE(w.flush());
		ASSERT_EQ(expectedLZ4pedSize, w.size());
		ASSERT_EQ(expectedLZ4pedSize, stream.size());
		for (int i = 0; i < n; ++i) {
			stream.writeUInt32(0xdeadbeef);
			stream.writeUInt32(0xbadc0ded);
			stream.writeUInt32(0xcafebabe);
			stream.writeUInt32(0xc001cafe);
		}
		ASSERT_EQ(size + expectedLZ4pedSize, stream.size());
	}
	stream.seek(0);
	{
		LZ4ReadStream r(stream, expectedLZ4pedSize);
		for (int i = 0; i < n; ++i) {
			int32_t val;
			ASSERT_FALSE(r.eos());
			ASSERT_EQ(0, r.readInt32(val)) << "unexpected read failure for step: " << i;
			ASSERT_EQ(val, i + 0) << "unexpected extracted value for step: " << i;
			ASSERT_FALSE(r.eos());
			ASSERT_EQ(0, r.readInt32(val)) << "unexpected read failure for step: " << i;
			ASSERT_EQ(val, i + 1) << "unexpected extracted value for step: " << i;
			ASSERT_FALSE(r.eos());
			ASSERT_EQ(0, r.readInt32(val)) << "unexpected read failure for step: " << i;
			ASSERT_EQ(val, i + 2) << "unexpected extracted value for step: " << i;
			ASSERT_FALSE(r.eos());
			ASSERT_EQ(0, r.readInt32(val)) << "unexpected read failure for step: " << i;
			ASSERT_EQ(val, i + 3) << "unexpected extracted value for step: " << i;
		}
		ASSERT_TRUE(r.eos());
		ASSERT_EQ(0, r.remaining());
		ASSERT_EQ(expectedLZ4pedSize, stream.pos());
		ASSERT_EQ(size, stream.remaining());
	}
}

TEST_F(LZ4StreamTest, testLZ4StreamBufSize) {
	const int n = 1;
	const int size = n * sizeof(uint32_t);
	BufferedReadWriteStream stream(size);
	{
		LZ4WriteStream w(stream);
		for (int i = 0; i < n; ++i) {
			ASSERT_TRUE(w.writeUInt32(i));
		}
	}
	stream.seek(0);
	LZ4ReadStream r(stream);
	uint8_t buf[16];
	EXPECT_EQ(4, r.read(buf, sizeof(buf)));
	EXPECT_EQ(0, r.read(buf, sizeof(buf)));
}

TEST_F(LZ4StreamTest, testLZ4StreamParentFailure) {
	const int n = 1;
	const int size = n * sizeof(uint32_t);
	BufferedReadWriteStream stream(size);
	{
		LZ4WriteStream w(stream);
		for (int i = 0; i < n; ++i) {
			ASSERT_TRUE(w.writeUInt32(i));
		}
	}
	stream.seek(0);
	LZ4ReadStream r(stream);
	stream.seek(0, SEEK_END);
	uint8_t buf[16];
	EXPECT_EQ(-1, r.read(buf, sizeof(buf)));
}

TEST_F(LZ4StreamTest, testIsLZ4StreamDetection) {
	const int n = 64;
	const int size = n * sizeof(uint32_t);
	BufferedReadWriteStream stream(size);
	{
		LZ4WriteStream w(stream);
		for (int i = 0; i < n; ++i) {
			ASSERT_TRUE(w.writeUInt32(i));
		}
	}
	stream.seek(0);
	EXPECT_TRUE(LZ4ReadStream::isLZ4Stream(stream));
	EXPECT_EQ(0u, stream.pos());
}

TEST_F(LZ4StreamTest, testMaybeLZ4StreamFalse) {
	const int n = 64;
	const int size = n * sizeof(uint32_t);
	BufferedReadWriteStream stream(size);
	for (int i = 0; i < n; ++i) {
		ASSERT_TRUE(stream.writeUInt32(i));
	}
	stream.seek(0);
	EXPECT_FALSE(LZ4ReadStream::isLZ4Stream(stream));
	EXPECT_EQ(0u, stream.pos());
}

} // namespace io

#endif // USE_LZ4
