/**
 * @file
 */

#include "io/Z85.h"
#include "core/tests/TestHelper.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryReadStream.h"
#include <gtest/gtest.h>

namespace io {

class Z85Test : public testing::Test {};

TEST_F(Z85Test, testZ85Encode) {
	const core::String input = "foobar";
	io::MemoryReadStream stream(input.c_str(), input.size());
	const core::String encoded = io::Z85::encode(stream);
	EXPECT_EQ("w]zP%vr8", encoded);
}

TEST_F(Z85Test, testZ85Decode) {
	const core::String input = "w]zP%vr8";
	io::BufferedReadWriteStream stream;
	EXPECT_TRUE(io::Z85::decode(stream, input));
	EXPECT_EQ(6u, stream.size());

	char strbuff[7];
	stream.seek(0);
	EXPECT_TRUE(stream.readString((int)(sizeof(strbuff) - 1), strbuff, false));
	strbuff[6] = '\0';
	EXPECT_STREQ("foobar", strbuff);
}

} // namespace io
