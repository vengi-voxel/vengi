/**
 * @file
 */

#include "io/StdStreamBuf.h"
#include "core/ArrayLength.h"
#include "io/BufferedReadWriteStream.h"
#include <gtest/gtest.h>

namespace io {

class StdStreamBufTest : public testing::Test {};

TEST_F(StdStreamBufTest, testOStream) {
	io::BufferedReadWriteStream target;
	StdOStreamBuf buf(target);
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

TEST_F(StdStreamBufTest, testIStream) {
	io::BufferedReadWriteStream target;
	target.writeString("foobar", false);
	StdIStreamBuf buf(target);
	std::istream istream(&buf);
	target.seek(0);
	EXPECT_EQ(0, target.pos());
	std::string str;
	istream >> str;
	EXPECT_EQ(str, "foobar");
}

} // namespace io
