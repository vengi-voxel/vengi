/**
 * @file
 */

#include "io/StdStreamBuf.h"
#include "core/ArrayLength.h"
#include "io/BufferedReadWriteStream.h"
#include <gtest/gtest.h>

namespace io {

class StdStreamBufTest : public testing::Test {};

TEST_F(StdStreamBufTest, testReadStream) {
	io::BufferedReadWriteStream target;
	StdStreamBuf buf(target);
	std::ostream ostream(&buf);
	EXPECT_EQ(0, target.pos());
	ostream << "test";
	EXPECT_EQ(4, target.size());
	EXPECT_EQ(4, target.pos());
	target.seek(0);
	char strbuf[5];
	target.readString(sizeof(strbuf) - 1, strbuf);
	strbuf[4] = '\0';
	EXPECT_STREQ("test", strbuf);
}

} // namespace io
