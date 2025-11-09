/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "io/BufferedReadWriteStream.h"
#include "io/ZipReadStream.h"
#include "io/ZipWriteStream.h"
#include <gtest/gtest.h>

namespace io {

class ZipStreamTest : public app::AbstractTest {};

TEST_F(ZipStreamTest, testZipStreamWrite) {
	BufferedReadWriteStream stream(1024);
	{
		ZipWriteStream w(stream);
		for (int i = 0; i < 64; ++i) {
			ASSERT_TRUE(w.writeInt32(i + 0)) << "unexpected write failure for step: " << i;
			ASSERT_TRUE(w.writeInt32(i + 1)) << "unexpected write failure for step: " << i;
			ASSERT_TRUE(w.writeInt32(i + 2)) << "unexpected write failure for step: " << i;
			ASSERT_TRUE(w.writeInt32(i + 3)) << "unexpected write failure for step: " << i;
		}
		ASSERT_TRUE(w.flush());
	}
}

TEST_F(ZipStreamTest, testZipStreamWriteAndRead) {
	BufferedReadWriteStream stream(1024);
	{
		ZipWriteStream w(stream);
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
		ZipReadStream r(stream, size);
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
		ZipReadStream r(stream, size);
		uint8_t buffer[64 * 4 * 4 + 10]; // bigger
		ASSERT_EQ(64 * 4 * 4, r.read(buffer, sizeof(buffer)));
		ASSERT_EQ(0, r.read(buffer, sizeof(buffer)));
		ASSERT_EQ(0, r.read(buffer, sizeof(buffer)));
	}
}

TEST_F(ZipStreamTest, testZipStreamWriteAndReadDeflate) {
	BufferedReadWriteStream stream(1024);
	{
		ZipWriteStream w(stream, 6, true);
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
		ZipReadStream r(stream, size);
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
		ZipReadStream r(stream, size);
		uint8_t buffer[64 * 4 * 4 + 10]; // bigger
		ASSERT_EQ(64 * 4 * 4, r.read(buffer, sizeof(buffer)));
		ASSERT_EQ(0, r.read(buffer, sizeof(buffer)));
		ASSERT_EQ(0, r.read(buffer, sizeof(buffer)));
	}
}

TEST_F(ZipStreamTest, testZipStreamNoSize) {
	const int n = 64;
	const int size = n * 4 * sizeof(uint32_t);
	const int expectedZippedSize = 134;
	BufferedReadWriteStream stream(size * 2);
	{
		ZipWriteStream w(stream);
		for (int i = 0; i < n; ++i) {
			ASSERT_TRUE(w.writeInt32(i + 0)) << "unexpected write failure for step: " << i;
			ASSERT_TRUE(w.writeInt32(i + 1)) << "unexpected write failure for step: " << i;
			ASSERT_TRUE(w.writeInt32(i + 2)) << "unexpected write failure for step: " << i;
			ASSERT_TRUE(w.writeInt32(i + 3)) << "unexpected write failure for step: " << i;
		}
		ASSERT_TRUE(w.flush());
		ASSERT_EQ(expectedZippedSize, w.size());
		ASSERT_EQ(expectedZippedSize, stream.size());
		for (int i = 0; i < n; ++i) {
			stream.writeUInt32(0xdeadbeef);
			stream.writeUInt32(0xbadc0ded);
			stream.writeUInt32(0xcafebabe);
			stream.writeUInt32(0xc001cafe);
		}
		ASSERT_EQ(size + expectedZippedSize, stream.size());
	}
	stream.seek(0);
	{
		ZipReadStream r(stream, expectedZippedSize);
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
		ASSERT_EQ(expectedZippedSize, stream.pos());
		ASSERT_EQ(size, stream.remaining());
	}
}

TEST_F(ZipStreamTest, testZipStreamBufSize) {
	const int n = 1;
	const int size = n * sizeof(uint32_t);
	BufferedReadWriteStream stream(size);
	{
		ZipWriteStream w(stream);
		for (int i = 0; i < n; ++i) {
			ASSERT_TRUE(w.writeUInt32(i));
		}
	}
	stream.seek(0);
	ZipReadStream r(stream);
	uint8_t buf[16];
	EXPECT_EQ(4, r.read(buf, sizeof(buf)));
	EXPECT_EQ(0, r.read(buf, sizeof(buf)));
}

TEST_F(ZipStreamTest, testZipStreamParentFailure) {
	const int n = 1;
	const int size = n * sizeof(uint32_t);
	BufferedReadWriteStream stream(size);
	{
		ZipWriteStream w(stream);
		for (int i = 0; i < n; ++i) {
			ASSERT_TRUE(w.writeUInt32(i));
		}
	}
	stream.seek(0);
	ZipReadStream r(stream);
	stream.seek(0, SEEK_END);
	uint8_t buf[16];
	EXPECT_EQ(-1, r.read(buf, sizeof(buf)));
}

TEST_F(ZipStreamTest, testIsZipStreamZlib) {
	const int n = 64;
	const int size = n * sizeof(uint32_t);
	BufferedReadWriteStream stream(size);
	{
		ZipWriteStream w(stream);
		for (int i = 0; i < n; ++i) {
			ASSERT_TRUE(w.writeUInt32(i));
		}
	}
	stream.seek(0);
	EXPECT_TRUE(ZipReadStream::isZipStream(stream));
	EXPECT_EQ(0u, stream.pos());
}

TEST_F(ZipStreamTest, testMaybeZipStreamDeflate) {
	const int n = 64;
	const int size = n * sizeof(uint32_t);
	BufferedReadWriteStream stream(size);
	{
		ZipWriteStream w(stream, 6, true);
		for (int i = 0; i < n; ++i) {
			ASSERT_TRUE(w.writeUInt32(i));
		}
	}
	stream.seek(0);
	EXPECT_TRUE(ZipReadStream::isZipStream(stream));
	EXPECT_EQ(0u, stream.pos());
}

TEST_F(ZipStreamTest, testMaybeZipStreamFalse) {
	const int n = 64;
	const int size = n * sizeof(uint32_t);
	BufferedReadWriteStream stream(size);
	for (int i = 0; i < n; ++i) {
		ASSERT_TRUE(stream.writeUInt32(i));
	}
	stream.seek(0);
	EXPECT_FALSE(ZipReadStream::isZipStream(stream));
	EXPECT_EQ(0u, stream.pos());
}

} // namespace io
