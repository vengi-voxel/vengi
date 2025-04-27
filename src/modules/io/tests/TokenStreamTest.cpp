/**
 * @file
 */

#include "io/TokenStream.h"
#include <gtest/gtest.h>

namespace io {

class TokenStreamTest : public testing::Test {};

class TestTokenStream : public TokenStream {
public:
	using TokenStream::TokenStream;

	bool testIsComments(uint8_t c) {
		return this->isComment(c);
	}

	uint8_t testSkipUntil(uint8_t c, const char *end, core::String &content) {
		this->skipUntil(c, end, &content);
		return c;
	}
};

TEST_F(TokenStreamTest, testTokenStreamIsComment) {
	TestTokenStream ts1("/** ignore me */token1rget");
	EXPECT_TRUE(ts1.testIsComments('/'));
	TestTokenStream ts2("token1rget");
	EXPECT_FALSE(ts2.testIsComments('/'));
}

TEST_F(TokenStreamTest, testTokenStreamSkipUntil) {
	TestTokenStream ts("/** ignore me */token1");
	EXPECT_FALSE(ts.eos());
	core::String content;
	EXPECT_EQ('t', ts.testSkipUntil('/', "*/", content));
	EXPECT_EQ("/** ignore me */", content);
}

TEST_F(TokenStreamTest, testTokenStreamComment) {
	TokenStream ts("/** ignore me */token1");
	EXPECT_FALSE(ts.eos());
	EXPECT_EQ("token1", ts.next());
	EXPECT_TRUE(ts.eos());
}

TEST_F(TokenStreamTest, testTokenStreamComment2) {
	TokenStream ts("token1/** ignore me */token2");
	EXPECT_FALSE(ts.eos());
	EXPECT_EQ("token1", ts.next());
	EXPECT_FALSE(ts.eos());
	EXPECT_EQ("token2", ts.next());
	EXPECT_TRUE(ts.eos());
}

TEST_F(TokenStreamTest, testTokenStream) {
	TokenStream ts(R"(
 token1
/** ignore me */
token2)");
	EXPECT_FALSE(ts.eos());
	EXPECT_EQ("token1", ts.next());
	EXPECT_FALSE(ts.eos());
	EXPECT_EQ("token2", ts.next());
	EXPECT_TRUE(ts.eos());
}

TEST_F(TokenStreamTest, testTokenStream2) {
	TokenStream ts(R"(
	token1  token2 token3 "token4"
// comment skip
 token5
/** ignore me
*/
token6)");
	EXPECT_FALSE(ts.eos());
	EXPECT_EQ("token1", ts.next());
	EXPECT_FALSE(ts.eos());
	EXPECT_EQ("token2", ts.next());
	EXPECT_FALSE(ts.eos());
	EXPECT_EQ("token3", ts.next());
	EXPECT_FALSE(ts.eos());
	EXPECT_EQ("token4", ts.next());
	EXPECT_FALSE(ts.eos());
	EXPECT_EQ("token5", ts.next());
	EXPECT_FALSE(ts.eos());
	EXPECT_EQ("token6", ts.next());
	EXPECT_TRUE(ts.eos());
}

} // namespace io
