/**
 * @file
 */

#include "io/BufferedReadWriteStream.h"
#include "io/ZipReadStream.h"
#include "io/ZipWriteStream.h"
#include <gtest/gtest.h>

namespace io {

class ZipStreamTest : public testing::Test {};

TEST_F(ZipStreamTest, testZipStream) {
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
}

TEST_F(ZipStreamTest, testZipStreamNoSize) {
	const int n = 64;
	const int size = n * 4 * sizeof(uint32_t);
	const int expectedZippedSize = 134;
	BufferedReadWriteStream stream(size);
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

} // namespace io
