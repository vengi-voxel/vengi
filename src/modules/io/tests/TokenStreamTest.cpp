/**
 * @file
 */

#include "io/TokenStream.h"
#include "io/BufferedReadWriteStream.h"
#include <gtest/gtest.h>

namespace io {

class TokenStreamTest : public testing::Test {};

TEST_F(TokenStreamTest, testTokenStream) {
	io::BufferedReadWriteStream target;
	target.writeString("token1  token2 token3", false);
	target.seek(0);
	TokenStream ts(target);
	EXPECT_FALSE(ts.eos());
	EXPECT_EQ("token1", ts.next());
	EXPECT_FALSE(ts.eos());
	EXPECT_EQ("token2", ts.next());
	EXPECT_FALSE(ts.eos());
	EXPECT_EQ("token3", ts.next());
	EXPECT_TRUE(ts.eos());
}

} // namespace io
