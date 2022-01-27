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
	stream.seek(0);
	{
		ZipReadStream r(stream);
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

} // namespace io
